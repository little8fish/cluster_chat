/**
 * 业务模块 用到了很多c++新特性编程 比如unordered_map 绑定器 绑定器绑定了消息id和消息回调函数 当网络模块收到一个消息请求时
 * 会解析消息 拿到消息id 再调用相应的回调函数处理消息
*/
#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "user.hpp"
#include <vector>

using namespace std;
using namespace muduo;

// 构造函数设为private 通过instance获取单例
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
// 初始化 消息 -> 业务函数
ChatService::ChatService()
{
    // 登录业务函数 枚举类型转化为整形
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 退出登录业务函数
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    // 注册业务函数
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // 双人聊天业务函数
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    // 添加好友业务函数
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 创建群组业务函数
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    // 群组添加成员业务函数
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    // 群聊业务函数
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器 监控消息
    if (_redis.connect())
    {
        // 绑定Redis消息处理函数
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}
// 重置
void ChatService::reset()
{
    // 通过用户模型重置用户状态
    _userModel.resetState();
}
// 根据消息类型获取业务处理函数
MsgHandler ChatService::getHandler(int msgid)
{

    auto it = _msgHandlerMap.find(msgid);
    // 没找到
    if (it == _msgHandlerMap.end())
    {

        // lambda表达式
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << " cant not find handler!";
        };
    }
    else
    {
        // 找到了直接返回该 业务处理函数
        return _msgHandlerMap[msgid];
    }
}
// 登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "Do not login repeatedly!";
            conn->send(response.dump());
        }
        // 成功登录
        else
        {
            {
                lock_guard<mutex> lock(_connMutex);
                // 绑定用户的id 和 conn
                _userConnMap.insert({id, conn});
            }

            // 订阅 
            _redis.subscribe(id);

            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                // json格式允许有vector
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    // 序列化
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        // 序列化
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    // 序列化
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "user name or password error!";
        conn->send(response.dump());
    }
}

// 退出登录业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    _redis.unsubscribe(userid);

    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);

    bool state = _userModel.insert(user);
    json response;
    if (state)
    {
        // 成功 回送消息
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 客户关闭异常 用于server处理连接
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    _redis.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 双人聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        // 找到了直接发送 通过conn发送
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }
    // 在user表中查看是否上线
    User user = _userModel.query(toid);
    // 如果上线了 说明这个用户连接别的服务器 通过redis把消息发布出去
    if (user.getState() == "online")
    {
        // 发布消息
        _redis.publish(toid, js.dump());
        return;
    }
    // 否则 离线消息 其实是放在离线消息表中
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 创建成功 设置群主
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 获取到除了发送者外群组里的其他用户
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    // 对每个用户 发送消息
    for (int id : useridVec)
    {
        lock_guard<mutex> lock(_connMutex);
        // 添加了发言者的id和name 还添加了时间
        auto it = _userConnMap.find(id);
        // 如果当前服务器里面保存了用户的conn 直接发送
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        // 没保存
        else
        {
            User user = _userModel.query(id);
            // 查询用户在不在线 在线 通过redis发布订阅功能发送
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            // 不在线 放入离线消息
            else
            {
                // 离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 服务器订阅了 用户id 的这个通道 当别的服务器上的用户给该用户发送消息 
// 该服务器会收到消息 然后查找该用户 给该用户转发消息 这就实现了跨服务器通信的问题
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    // 用户没上线 存为离线消息
    _offlineMsgModel.insert(userid, msg);
}