#ifndef GROUP_H
#define GROUP_H
#include <string>
#include <vector>
#include "groupuser.hpp"

using namespace std;

class Group
{
public:
    Group(int aid = -1, string aname = "", string adesc = "")
        : id(aid), name(aname), desc(adesc) {}
    void setId(int id)
    {
        this->id = id;
    }
    void setName(string name)
    {
        this->name = name;
    }
    void setDesc(string desc)
    {
        this->desc = desc;
    }
    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUsers() { return this->users; }

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;
};

#endif