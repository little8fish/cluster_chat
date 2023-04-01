#include "groupmodel.hpp"
#include "db.h"

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup (groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a \
    join groupuser b on a.id = b.groupid where b.userid = %d",
            userid);

    MySQL mysql;
    vector<Group> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for(auto &group: vec){
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a join \
        groupuser b on a.id = b.userid where b.groupid = %d", group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr){
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))!= nullptr){
                GroupUser gp;
                gp.setId(atoi(row[0]));
                gp.setName(row[1]);
                gp.setState(row[2]);
                gp.setRole(row[3]);
                group.getUsers().push_back(gp);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser \
    where groupid = %d and userid != %d", groupid, userid);

    MySQL mysql;
    if (mysql.connect()){
        MYSQL_RES *res =  mysql.query(sql);
        if (res!=nullptr){
            MYSQL_ROW row;
            vector<int> vec;
            while ((row = mysql_fetch_row(res))!=nullptr){
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vector<int>();
}
