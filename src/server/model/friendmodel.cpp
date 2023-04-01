#include "friendmodel.hpp"
#include "db.h"

void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    // 为什么用到内连接 这样就可以通过friend表查到user表
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on a.id = b.friendid where b.userid = %d", userid);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res;
        if ((res = mysql.query(sql)) != nullptr){
            MYSQL_ROW row;
            vector<User> vec; // 要用到时再定义
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vector<User>();
}