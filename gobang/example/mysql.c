#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>

#define HOST "127.0.0.1" // MySQL服务器地址
#define PORT 3306        // MySQL服务器端口
#define USER "root"      // MySQL用户名
#define PASS "123456"    // MySQL密码
#define DBNAME "gobang"  // 数据库名

int main()
{
    MYSQL *mysql = mysql_init(NULL); // 初始化MySQL连接
    if (mysql == NULL)
    {
        printf("mysql init failed!\n"); // 初始化失败
        return -1;
    }
    if (mysql_real_connect(mysql, HOST, USER, PASS, DBNAME, PORT, NULL, 0) == NULL)
    {
        printf("connect mysql server failed:%s\n", mysql_error(mysql)); // 连接失败
        mysql_close(mysql);                                             // 关闭连接
        return -1;
    }
    if (mysql_set_character_set(mysql, "utf8") != 0)
    {
        printf("set client character failed:%s\n", mysql_error(mysql)); // 设置字符集失败
        mysql_close(mysql);                                             // 关闭连接
        return -1;
    }
    // SQL查询语句
    char *sql = "select * from stu;";
    int ret = mysql_query(mysql, sql); // 执行SQL查询
    if (ret != 0)
    {
        printf("%s\n", sql);                                   // 打印SQL语句
        printf("mysql_query failed:%s\n", mysql_error(mysql)); // 查询失败
        mysql_close(mysql);                                    // 关闭连接
        return -1;
    }
    MYSQL_RES *res = mysql_store_result(mysql); // 获取查询结果
    if (res == NULL)
    {
        mysql_close(mysql); // 关闭连接
        return -1;
    }
    int num_row = mysql_num_rows(res);   // 获取结果行数
    int num_col = mysql_num_fields(res); // 获取结果列数
    for (int i = 0; i < num_row; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(res); // 获取下一行数据
        for (int j = 0; j < num_col; j++)
        {
            printf("%s\t", row[j]); // 打印列数据
        }
        printf("\n"); // 换行
    }
    mysql_free_result(res); // 释放结果集
    mysql_close(mysql);     // 关闭连接
    return 0;
}
