#ifndef __M_SRV_H__
#define __M_SRV_H__
#include "db.hpp"
#include "matcher.hpp"
#include "online.hpp"
#include "room.hpp"
#include "session.hpp"
#include "util.hpp"
#define WWWROOT "./wwwroot/"
class gobang_server
{
private:
    std::string _web_root;
    wsserver_t _wssrv;
    user_table _ut;
    online_manager _om;
    room_manager _rm;
    matcher _mm;
    session_manager _sm;

private:
    void http_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req=conn->get_request();
        std::string method=req.get_method();
        std::string uri=req.get_uri();
        std::string pathname = _web_root + uri;
        std::string body;
        file_util::read(pathname, body);
        conn->set_status(websocketpp::http::status_code::ok);
        conn->set_body(body);
    }
    void wsopen_callback(websocketpp::connection_hdl hdl)
    {
    }
    void wsclose_callback(websocketpp::connection_hdl hdl)
    {
    }
    void wsmsg_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
    {
    }

public:
    gobang_server(const std::string &host,
                  const std::string &user,
                  const std::string &pass,
                  const std::string &dbname,
                  uint16_t port = 3306,
                  const std::string &wwwroot = WWWROOT) : _web_root(wwwroot), _ut(host, user, pass, dbname, port),
                                                          _rm(&_ut, &_om), _sm(&_wssrv), _mm(&_rm, &_ut, &_om)
    {
        _wssrv.set_access_channels(websocketpp::log::alevel::none);
        _wssrv.init_asio();
        _wssrv.set_reuse_addr(true);
        _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1));
        _wssrv.set_open_handler(std::bind(&gobang_server::wsopen_callback, this, std::placeholders::_1));
        _wssrv.set_close_handler(std::bind(&gobang_server::wsclose_callback, this, std::placeholders::_1));
        _wssrv.set_message_handler(std::bind(&gobang_server::wsmsg_callback, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start(int port)
    {
        _wssrv.listen(port);
        _wssrv.start_accept();
        _wssrv.run();
    }
};
#endif