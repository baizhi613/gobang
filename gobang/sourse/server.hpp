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
    void file_handler(wsserver_t::connection_ptr &conn)
    {
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        std::string realpath = _web_root + uri;
        if (realpath.back() == '/')
        {
            realpath += "login.html";
        }
        std::string body;
        bool ret = file_util::read(realpath, body);
        if (ret == false)
        {
            body += "<html>";
            body += "<head>";
            body += "<meta charset=' UTF - 8 '/>";
            body += "</head>";
            body += "<body>";
            body += "<h1> Not Found </h1>";
            body += "</body>";
            conn->set_status(websocketpp::http::status_code::not_found);
            conn->set_body(body);
            return;
        }
        conn->set_body(body);
        conn->set_status(websocketpp::http::status_code::ok);
    }
    void http_resp(wsserver_t::connection_ptr &conn, bool result, websocketpp::http::status_code::value code, const std::string &reason)
    {
        Json::Value resp_json;
        resp_json["result"] = result;
        resp_json["reason"] = reason;
        std::string resp_body;
        json_util::serialize(resp_json, resp_body);
        conn->set_status(code);
        conn->set_body(resp_body);
        conn->append_header("Content-Type", "application/json");
        return;
    }
    void reg(wsserver_t::connection_ptr &conn)
    {
        websocketpp::http::parser::request req = conn->get_request();
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        Json::Value resp_json;
        bool ret = json_util::unserialize(req_body, login_info);
        if (ret == false)
        {
            DLOG("反序列注册信息失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式有误");
        }
        if (login_info["username"].isNull() || login_info["password"].isNull())
        {
            DLOG("用户名密码不完整");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        }
        ret = _ut.insert(login_info);
        if (ret == false)
        {
            DLOG("向数据库插入数据失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名已经被占用");
        }
        return http_resp(conn, true, websocketpp::http::status_code::ok, "注册用户成功");
    }
    void login(wsserver_t::connection_ptr &conn)
    {
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        bool ret = json_util::unserialize(req_body, login_info);
        if (ret == false)
        {
            DLOG("反序列注册信息失败");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式有误");
        }
        if (login_info["username"].isNull() || login_info["password"].isNull())
        {
            DLOG("用户名密码不完整");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        }
        ret = _ut.login(login_info);
        if (ret == false)
        {
            DLOG("用户名密码错误");
            std::cout << login_info;
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名密码错误");
        }
        uint64_t uid = login_info["id"].asUInt64();
        session_ptr ssp = _sm.create_session(uid, LOGIN);
        if (ssp.get() == nullptr)
        {
            DLOG("创建会话失败");
            return http_resp(conn, false, websocketpp::http::status_code::internal_server_error, "创建会话失败");
        }
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        std::string cookie_ssid = "SSID=" + std::to_string(ssp->ssid());
        conn->append_header("Set-Cookie", cookie_ssid);
        return http_resp(conn, true, websocketpp::http::status_code::ok, "登录成功");
    }
    bool get_cookie_val(const std::string &cookie_str, const std::string &key, std::string &val)
    {
        std::string sep = "; ";
        std::vector<std::string> cookie_arr;
        string_util::split(cookie_str, sep, cookie_arr);
        for (auto str : cookie_arr)
        {
            std::vector<std::string> tmp_arr;
            string_util::split(str, "=", tmp_arr);
            if (tmp_arr.size() != 2)
            {
                continue;
            }
            if (tmp_arr[0] == key)
            {
                val = tmp_arr[1];
                return true;
            }
        }
        return false;
    }
    void info(wsserver_t::connection_ptr &conn)
    {
        Json::Value err_resp;
        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到cookie信息,请重新登录");
        }
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (ret == false)
        {
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到ssid信息,请重新登录");
        }
        session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
        if (ssp.get() == nullptr)
        {
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "登录过期,请重新登录");
        }
        uint64_t uid = ssp->get_user();
        Json::Value user_info;
        ret = _ut.select_by_id(uid, user_info);
        if (ret == false)
        {
            return http_resp(conn, true, websocketpp::http::status_code::bad_request, "找不到用户信息,请重新登录");
        }
        std::string body;
        json_util::serialize(user_info, body);
        conn->set_body(body);
        conn->append_header("Content-Type", "application/json");
        conn->set_status(websocketpp::http::status_code::ok);
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }
    void http_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string method = req.get_method();
        std::string uri = req.get_uri();
        if (method == "POST" && uri == "/reg")
        {
            return reg(conn);
        }
        else if (method == "POST" && uri == "/login")
        {
            return login(conn);
        }
        else if (method == "GET" && uri == "/info")
        {
            return info(conn);
        }
        else
        {
            return file_handler(conn);
        }
    }
    void ws_resp(wsserver_t::connection_ptr conn, Json::Value &resp)
    {
        std::string body;
        json_util::serialize(resp, body);
        conn->send(body);
    }
    session_ptr get_session_by_cookie(wsserver_t::connection_ptr conn)
    {
        Json::Value err_resp;
        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到cookie信息,需要重新登录";
            err_resp["result"] = false;
            ws_resp(conn, err_resp);
            return session_ptr();
        }
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (ret == false)
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到SSID信息,需要重新登录";
            err_resp["result"] = false;
            ws_resp(conn, err_resp);
            return session_ptr();
        }
        session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
        if (ssp.get() == nullptr)
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到session信息,需要重新登录";
            err_resp["result"] = false;
            ws_resp(conn, err_resp);
            return session_ptr();
        }
        return ssp;
    }
    void wsopen_game_hall(wsserver_t::connection_ptr conn)
    {
        Json::Value resp_json;
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
        {
            return;
        }
        if (_om.is_in_game_hall(ssp->get_user() || _om.is_in_game_room(ssp->get_user())))
        {
            resp_json["optype"] = "hall_ready";
            resp_json["reason"] = "玩家重复登录!";
            resp_json["result"] = false;
            return ws_resp(conn, resp_json);
        }
        _om.enter_game_hall(ssp->get_user(), conn);
        resp_json["optype"] = "hall_ready";
        resp_json["result"] = true;
        ws_resp(conn, resp_json);
        _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
    }
    void wsopen_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            return wsopen_game_hall(conn);
        }
        else if (uri == "/room")
        {
        }
    }
    void wsclose_game_hall(wsserver_t::connection_ptr conn)
    {
        Json::Value err_resp;
        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到cookie信息,需要重新登录";
            err_resp["result"] = false;
            return ws_resp(conn, err_resp);
        }
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (ret == false)
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到SSID信息,需要重新登录";
            err_resp["result"] = false;
            return ws_resp(conn, err_resp);
        }
        session_ptr ssp = _sm.get_session_by_ssid(std::stol(ssid_str));
        if (ssp.get() == nullptr)
        {
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到session信息,需要重新登录";
            err_resp["result"] = false;
            return ws_resp(conn, err_resp);
        }
        _om.exit_game_hall(ssp->get_user());
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }
    void wsclose_callback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            return wsclose_game_hall(conn);
        }
        else if (uri == "/room")
        {
        }
    }
    void wsmsg_game_hall(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg)
    {
        Json::Value resp_json;
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
        {
            return;
        }
        std::string req_body = msg->get_payload();
        Json::Value req_json;
        bool ret = json_util::unserialize(req_body, req_json);
        if (ret == false)
        {
            resp_json["result"] = false;
            resp_json["reason"] = "请求信息解析失败";
            return ws_resp(conn, resp_json);
        }
        if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_start")
        {
            _mm.add(ssp->get_user());
            resp_json["optype"] = "match_start";
            resp_json["result"] = true;
            return ws_resp(conn, resp_json);
        }
        else if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_stop")
        {
            _mm.del(ssp->get_user());
            resp_json["optype"] = "match_stop";
            resp_json["result"] = true;
            return ws_resp(conn, resp_json);
        }
        resp_json["optype"] = "unknow";
        resp_json["reason"] = "请求类型未知";
        resp_json["result"] = false;
        return ws_resp(conn, resp_json);
    }
    void wsmsg_callback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hdl);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        {
            return wsmsg_game_hall(conn, msg);
        }
        else if (uri == "/room")
        {
        }
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