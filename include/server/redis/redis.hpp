#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    bool connect();

    bool publish(int channel, string message);  

    bool subscribe(int channel);

    bool unsubscribe(int channel);

    void observer_channel_message();

    void init_notify_handler(function<void(int, string)> fn);

private:
    redisContext *_publish_context;
    
    // 订阅通道 正常订阅会阻塞等待接收 项目中把订阅命令发送出去后 不等待接收 接收和处理由单独的接收线程完成
    redisContext *_subcribe_context;
    // 收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;
};

#endif