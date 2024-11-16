#ifndef __M_ROOM_H__
#define __M_ROOM_H__
#include "util.hpp"
#include "db.hpp"
#include "online.hpp"
#include "logger.hpp"
#define BOARD_ROW 15
#define BOARD_COL 15
#define CHESS_WHITE 1
#define CHESS_BLACK 2
typedef enum
{
    GAME_START,
    GAME_OVER
} room_statu;
class room
{
private:
    uint64_t _room_id;
    room_statu _statu;
    int _player_count;
    uint64_t _white_id;
    uint64_t _black_id;
    user_table *_tb_user;
    online_manager *_online_user;
    std::vector<std::vector<int>> _board;

private:
    bool five(int row, int col, int row_off, int col_off, int color)
    {
        int count = 1;
        int search_row = row + row_off;
        int search_col = col + col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;
            search_row += row_off;
            search_col += col_off;
        }
        search_row = row - row_off;
        search_col = col - col_off;
        while (search_row >= 0 && search_row < BOARD_ROW &&
               search_col >= 0 && search_col < BOARD_COL &&
               _board[search_row][search_col] == color)
        {
            count++;
            search_row -= row_off;
            search_col -= col_off;
        }
        return (count >= 5);
    }
    uint64_t check_win(int row, int col, int color)
    {
        if (five(row, col, 0, 1, color) ||
            five(row, col, 1, 0, color) ||
            five(row, col, -1, 1, color) ||
            five(row, col, -1, -1, color))
        {
            return color == CHESS_WHITE ? _white_id : _black_id;
        }
        return 0;
    }

public:
    room(uint64_t room_id, user_table *tb_user, online_manager *online_user) : _room_id(room_id), _statu(GAME_START), _player_count(0),
                                                                               _tb_user(tb_user), _online_user(online_user),
                                                                               _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))
    {
        DLOG("%lu 房间创建成功!!", _room_id);
    }
    ~room()
    {
        DLOG("%lu 房间销毁成功!!", _room_id);
    }
    uint64_t id()
    {
        return _room_id;
    }
    room_statu statu()
    {
        return _statu;
    }
    int player_count()
    {
        return _player_count;
    }
    void add_white_user(uint64_t uid)
    {
        _white_id = uid;
        _player_count++;
    }
    void add_black_user(uint64_t uid)
    {
        _black_id = uid;
        _player_count++;
    }
    uint64_t get_white_user()
    {
        return _white_id;
    }
    uint64_t get_black_user()
    {
        return _black_id;
    }
    Json::Value handle_chess(Json::Value &req)
    {
        Json::Value json_resp = req;
        int chess_row = req["row"].asInt();
        int chess_col = req["col"].asInt();
        uint64_t cur_uid = req["uid"].asUInt64();
        if (_online_user->is_in_game_room(_white_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好!对方掉线,不战而胜!";
            json_resp["winner"] = (Json::UInt64)_black_id;
            return json_resp;
        }
        if (_online_user->is_in_game_room(_black_id) == false)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "运气真好!对方掉线,不战而胜!";
            json_resp["winner"] = (Json::UInt64)_white_id;
            return json_resp;
        }
        if (_board[chess_row][chess_col] != 0)
        {
            json_resp["result"] = true;
            json_resp["reason"] = "当前位置已经有了其他棋子!";
            return json_resp;
        }
        int cur_color = cur_uid == _white_id ? CHESS_WHITE : CHESS_BLACK;
        _board[chess_row][chess_col] = cur_color;
        uint64_t winner_id = check_win(chess_row, chess_col, cur_color);
        if (winner_id != 0)
        {
            json_resp["reason"] = "五星连珠,战无敌!";
        }
        json_resp["result"] = true;
        json_resp["winner"] = (Json::UInt64)winner_id;
        return json_resp;
    }
    Json::Value handle_chat(Json::Value &req)
    {
        Json::Value json_resp = req;
        std::string msg = req["message"].asCString();
        size_t pos = msg.find("垃圾");
        if (pos != std::string::npos)
        {
            json_resp["result"] = false;
            json_resp["reason"] = "消息中包含敏感词,不能发送!";
            return json_resp;
        }
        json_resp["result"] = true;
        return json_resp;
    }
    void handle_exit(uint64_t uid)
    {
        Json::Value json_resp;
        if (_statu == GAME_START)
        {
            json_resp["optype"] = "put_chess";
            json_resp["result"] = true;
            json_resp["reason"] = "对方掉线,不战而胜!";
            json_resp["room_id"] = (Json::UInt64)_room_id;
            json_resp["uid"] = (Json::UInt64)uid;
            json_resp["row"] = -1;
            json_resp["col"] = -1;
            json_resp["winner"] = (Json::UInt64)(uid == _white_id ? _black_id : _white_id);
            broadcast(json_resp);
        }
        _player_count--;
        return;
    }
    void handle_request(Json::Value &req)
    {
        Json::Value json_resp = req;
        uint64_t room_id = req["room_id"].asUInt64();
        if (room_id != _room_id)
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "房间号不匹配";
            return broadcast(json_resp);
        }
        if (req["optype"].asString() == "put_chess")
        {
            json_resp = handle_chess(req);
            if (json_resp["winner"].asUInt64() != 0)
            {
                uint64_t winner_id = json_resp["winner"].asUInt64();
                uint64_t loser_id = winner_id == _white_id ? _black_id : _white_id;
                _tb_user->win(_black_id);
                _tb_user->lose(_white_id);
                _statu = GAME_OVER;
            }
        }
        else if (req["optype"].asString() == "chat")
        {
            json_resp = handle_chat(req);
        }
        else
        {
            json_resp["optype"] = req["optype"].asString();
            json_resp["result"] = false;
            json_resp["reason"] = "未知请求类型";
        }
        return broadcast(json_resp);
    }
    void broadcast(Json::Value &rsp)
    {
        std::string body;
        json_util::serialize(rsp, body);
        wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
        if (wconn.get() != nullptr)
        {
            wconn->send(body);
        }
        wsserver_t::connection_ptr bconn = _online_user->get_conn_from_room(_black_id);
        if (wconn.get() != nullptr)
        {
            wconn->send(body);
        }
        return;
    }
};

using room_ptr = std::shared_ptr<room>;

class room_manager
{
private:
    uint64_t _next_rid;
    std::mutex _mutex;
    user_table *_tb_user;
    online_manager *_online_user;
    std::unordered_map<uint64_t, room_ptr> _rooms;
    std::unordered_map<uint64_t, uint64_t> _users;

public:
    room_manager(user_table *ut, online_manager *om) : _next_rid(1), _tb_user(ut), _online_user(om)
    {
        DLOG("房间管理模块初始化完毕!");
    }
    ~room_manager()
    {
        DLOG("房间管理模块即将销毁!");
    }
    room_ptr create_room(uint64_t uid1, uint64_t uid2)
    {
        if (_online_user->is_in_game_hall(uid1) == false)
        {
            DLOG("用户:%lu不在大厅中,创建房间失败!", uid1);
            return room_ptr();
        }
        if (_online_user->is_in_game_hall(uid2) == false)
        {
            DLOG("用户:%lu不在大厅中,创建房间失败!", uid2);
            return room_ptr();
        }
        std::unique_lock<std::mutex> lock(_mutex);
        room_ptr rp(new room(_next_rid, _tb_user, _online_user));
        rp->add_white_user(uid1);
        rp->add_black_user(uid2);
        _rooms.insert(std::make_pair(_next_rid, rp));
        _users.insert(std::make_pair(uid1, _next_rid));
        _users.insert(std::make_pair(uid2, _next_rid));
        _next_rid++;
        return rp;
    }
    room_ptr get_room_by_rid(uint64_t rid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _rooms.find(rid);
        if (it == _rooms.end())
        {
            return room_ptr();
        }
        return it->second;
    }
    room_ptr get_room_by_uid(uint64_t uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto uit = _users.find(uid);
        if (uit == _users.end())
        {
            return room_ptr();
        }
        uint64_t rid = uit->second;
        auto rit = _rooms.find(rid);
        if (rit == _rooms.end())
        {
            return room_ptr();
        }
        return rit->second;
    }
    void remove_room(uint64_t rid)
    {
        room_ptr rp = get_room_by_rid(rid);
        if (rp.get() == nullptr)
        {
            return;
        }
        uint64_t uid1 = rp->get_white_user();
        uint64_t uid2 = rp->get_black_user();
        std::unique_lock<std::mutex> lock(_mutex);
        _users.erase(uid1);
        _users.erase(uid2);
        _rooms.erase(rid);
    }
    void remove_room_user(uint64_t uid)
    {
        room_ptr rp = get_room_by_uid(uid);
        if (rp.get() == nullptr)
        {
            return;
        }
        rp->handle_exit(uid);
        if (rp->player_count() == 0)
        {
            remove_room(rp->id());
        }
    }
};
#endif
