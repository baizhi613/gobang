#ifndef __M_SS_H__
#define __M_SS_H__
#include "util.hpp"
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
#endif