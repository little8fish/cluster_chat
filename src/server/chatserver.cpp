#include "chatserver.hpp"
#include <functional>
#include "json.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 绑定 处理连接的回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 绑定 处理消息的回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置服务器的线程数
    _server.setThreadNum(4);
}
void ChatServer::start()
{
    // 服务器开始运行
    _server.start();
}
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 连接未建立时 
    if (!conn->connected()) {
        // 处理客户关闭异常
        ChatService::instance()->clientCloseException(conn);
        // 释放该连接
        conn->shutdown();
    }
}
void ChatServer::onMessage(const TcpConnectionPtr &conn,
               Buffer *buffer,
               Timestamp time)
{
    // 取出消息
    string buf = buffer->retrieveAllAsString();
    // 解析消息
    json js = json::parse(buf);
    // 根据消息类型 拿到相应的处理函数 （ 拿到了service类的业务处理 再进行调用 回调函数 ） 
    // 通过这种回调 就不用写大量if else
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 对消息处理
    msgHandler(conn, js, time);
}