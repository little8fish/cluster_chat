#ifndef USER_H
#define USER_H
#include <string>

using namespace std;

class User
{
public:
    User(int aid = -1, string aname = "", string apassword = "", string astate = "offline")
        : id(aid), name(aname), password(apassword), state(astate)
    {
    }
    void setId(int id)
    {
        this->id = id;
    }
    void setName(string name)
    {
        this->name = name;
    }
    void setPassword(string password)
    {
        this->password = password;
    }
    void setState(string state)
    {
        this->state = state;
    }
    int getId() const
    {
        return this->id;
    }
    string getName() const
    {
        return this->name;
    }
    string getPassword() const
    {
        return this->password;
    }
    string getState() const
    {
        return this->state;
    }

private:
    int id;
    string name;
    string password;
    string state;
};

#endif