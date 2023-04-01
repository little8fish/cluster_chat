#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <functional>
#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <json.hpp>
#include <mutex>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using json = nlohmann::json;

using namespace std;
using namespace muduo;
using namespace muduo::net;

// 函数指针类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

class ChatService
{
public:
    // 获取单例
    static ChatService *instance();
    // 登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 退出登录业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 双人聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 根据消息类型 获取业务处理函数
    MsgHandler getHandler(int msgid);
    // 客户断开异常处理
    void clientCloseException(const TcpConnectionPtr &conn);
    // 重置 （目前只是调用用户模型设置所有user下线状态）
    void reset();
    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群聊
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    // 构造函数设置为私有
    ChatService();
    // 映射 消息类型 -> 消息处理 
    unordered_map<int, MsgHandler> _msgHandlerMap;
    // 映射 user -> 连接 
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 处理用户的模型
    UserModel _userModel;
    // 锁_userConnMap
    mutex _connMutex;
    // 处理离线消息的模型
    OfflineMsgModel _offlineMsgModel;
    // 处理朋友的模型
    FriendModel _friendModel;
    // 处理群组的模型
    GroupModel _groupModel;
    // redis操作对象
    Redis _redis;
};

#endif