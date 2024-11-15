#include "util.hpp"
#include "db.hpp"
#include "online.hpp"
#include "room.hpp"
#define HOST "127.0.0.1" // MySQL服务器地址
#define PORT 3306        // MySQL服务器端口
#define USER "root"      // MySQL用户名
#define PASS "123456"    // MySQL密码
#define DBNAME "gobang"  // 数据库名
void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASS, DBNAME, PORT);
    const char *sql = "insert stu values(null,'小花',18,53,68,87);";
    bool ret = mysql_util::mysql_exec(mysql, sql);
    if (ret == false)
    {
        return;
    }
    mysql_util::mysql_destroy(mysql);
}
void json_test()
{
    Json::Value root;
    std::string body;
    root["姓名"] = "小明";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);
    json_util::serialize(root, body);
    DLOG("%s", body.c_str());

    Json::Value val;
    json_util::unserialize(body, val);
    std::cout << "姓名:" << val["姓名"].asString() << std::endl;
    std::cout << "年龄:" << val["年龄"].asInt() << std::endl;
    int sz = val["成绩"].size();
    for (int i = 0; i < sz; i++)
    {
        std::cout << "成绩:" << val["成绩"][i].asFloat() << std::endl;
    }
}
void str_test()
{
    std::string str = "123,234,,,,,345";
    std::vector<std::string> arry;
    string_util::split(str, ",", arry);
    for (auto s : arry)
    {
        DLOG("%s", s.c_str());
    }
}
void file_test()
{
    std::string filename="./makefile";
    std::string body;
    file_util::read(filename,body);
    std::cout<<body<<std::endl;
}
void db_test()
{
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    Json::Value user;
    //user["username"]="xiaoming";
    //user["password"]="123123";
    ut.win(5);
    bool ret=ut.select_by_id(5,user);
    std::string body;
    json_util::serialize(user,body);
    std::cout<<body<<std::endl;
}
void online_test()
{
    online_manager om;
    wsserver_t::connection_ptr conn;
    uint64_t uid=2;
    om.enter_game_hall(uid,conn);
    if(om.is_in_game_hall(uid))
    {
        DLOG("IN GAME HALL");
    }
    else
    {
        DLOG("NOT IN GAME HALL");
    }
    om.exit_game_hall(uid);
    if(om.is_in_game_hall(uid))
    {
        DLOG("IN GAME HALL");
    }
    else
    {
        DLOG("NOT IN GAME HALL");
    }
}
int main()
{
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    online_manager om;
    room r(10,&ut,&om);//游戏房间测试
    online_test();
    return 0;
}
