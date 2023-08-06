#include "db.h"
#include <muduo/base/Logging.h>
#include <iostream>

// 要先打开mysql  sudo service mysql start
static string server = "192.168.0.132";
static string user = "djc";
static string password = "123456";
static string dbname = "chat";

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
bool MySQL::connect()
{
    // 连接不上是为什么  是端口号的问题？  自己设置服务器端口开的是3307
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3307, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
    // 函数返回类型是bool 而p是MYSQL类型指针 是否 p不为空所以返回true
    return p;
}
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败！";
        cout << mysql_error(_conn) << endl;
        return false;
    }
    return true;
}
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {

        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
MYSQL *MySQL::getConnection()
{
    return _conn;
}
