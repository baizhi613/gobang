#ifndef __M_SS_H__
#define __M_SS_H__
#include "util.hpp"
#include <unordered_map>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;
class session
{
private:
    uint64_t _ssid;
    uint64_t _uid;
    ss_statu _statu;
    wsserver_t::timer_ptr _tp;

public:
    session(uint64_t ssid) : _ssid(ssid)
    {
        DLOG("SESSION %p 被创建!!", this);
    }
    ~session()
    {
        DLOG("SESSION %p 被释放!!", this);
    }
    uint64_t ssid()
    {
        return _ssid;
    }
    void set_statu(ss_statu statu)
    {
        _statu = statu;
    }
    void set_user(uint64_t uid)
    {
        _uid = uid;
    }
    uint64_t get_user()
    {
        return _uid;
    }
    bool is_login()
    {
        return (_statu == LOGIN);
    }
    void set_timer(const wsserver_t::timer_ptr &tp)
    {
        _tp = tp;
    }
    wsserver_t::timer_ptr &get_timer()
    {
        return _tp;
    }
};

#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr = std::shared_ptr<session>;
class session_manager
{
private:
    uint64_t _next_ssid;
    std::mutex _mutex;
    std::unordered_map<uint64_t, session_ptr> _session;
    wsserver_t *_server;

public:
    session_manager(wsserver_t *srv) : _next_ssid(1), _server(srv)
    {
        DLOG("session管理器初始化完毕!");
    }
    ~session_manager()
    {
        DLOG("session管理器即将销毁!");
    }
    session_ptr create_session(uint64_t uid, ss_statu statu)
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        session_ptr ssp(new session(_next_ssid));
        ssp->set_statu(statu);
        ssp->set_user(uid);
        _session.insert(std::make_pair(_next_ssid, ssp));
        _next_ssid++;
        return ssp;
    }
    void append_session(const session_ptr &ssp)
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        _session.insert(std::make_pair(ssp->ssid(), ssp));
    }
    session_ptr get_session_by_ssid(uint64_t ssid)
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        auto it = _session.find(ssid);
        if (it == _session.end())
        {
            return session_ptr();
        }
        return it->second;
    }
    void set_session_expire_time(uint64_t ssid, int ms)
    {
        session_ptr ssp = get_session_by_ssid(ssid);
        if (ssp.get() == nullptr)
        {
            return;
        }
        wsserver_t::timer_ptr tp = ssp->get_timer();
        if (tp.get() == nullptr && ms == SESSION_FOREVER)
        {
            return;
        }
        else if (tp.get() == nullptr && ms != SESSION_FOREVER)
        {
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms, std::bind(&session_manager::remove_session, this, ssid));
            ssp->set_timer(tmp_tp);
        }
        else if (tp.get() != nullptr && ms == SESSION_FOREVER)
        {
            tp->cancel();
            ssp->set_timer(wsserver_t::timer_ptr());
            _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));
        }
        else if(tp.get() != nullptr && ms != SESSION_FOREVER)
        {
            tp->cancel();
            ssp->set_timer(wsserver_t::timer_ptr());
            _server->set_timer(0,std::bind(&session_manager::append_session,this,ssp));
            ssp->set_timer(wsserver_t::timer_ptr());
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,std::bind(&session_manager::remove_session,this,ssp->ssid()));
            ssp->set_timer(tmp_tp);
        }
    }
    void remove_session(uint64_t ssid)
    {
        std::unique_lock<std::mutex> _lock(_mutex);
        _session.erase(ssid);
    }
};
#endif