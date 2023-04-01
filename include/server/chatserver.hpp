#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;
// 封装 server eventloop 用于处理连接及连接上的事件
// 一条一条连接过来
class ChatServer
{
public:
    // 绑定两个回调函数 
    ChatServer(EventLoop *,
               const InetAddress &,
               const string &);
    // _server开始服务
    void start();

private:
    // 回调函数 处理用户连接的建立和断开
    void onConnection(const TcpConnectionPtr &);
    // 回调函数 处理用户的消息
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);
    // 一个tcp连接服务端
    TcpServer _server;
    // 事件循环
    EventLoop *_loop;
};
#endif