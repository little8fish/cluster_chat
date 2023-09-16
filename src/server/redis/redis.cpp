/**
 * 单机服务下 就是这些模块 其并发能力有限
 * 所以采用了能快速提高并发能力的多机扩展的方式 
 * 那要在多台服务器上部署的话
 * 就要支持负载均衡
 * 所以配置了nginx基于tcp的负载均衡
 * 
 * 同时 多机扩展方式下 客户连接到不同服务器上 可能会跨服务器通信 所以引入了redis作为消息队列 利用redis的发布订阅功能解决了跨服务器通信的问题
 * 
*/
#include "redis.hpp"
#include <iostream>


// 发布通道 订阅通道
Redis::Redis() : _publish_context(nullptr), _subcribe_context(nullptr)
{
}
Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (_subcribe_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    

    // 开一个线程 监控消息 
    // 因为通常订阅之后 默认是阻塞的 
    // 然后这里的订阅会仅订阅 不阻塞等待回复 用一个接收线程等待所有回复
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success!" << endl;
    return true;
}

// 发布消息
bool Redis::publish(int channel, string message)
{
    // 直接redisCommand
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 订阅
bool Redis::subscribe(int channel)
{
    // 使用redisCommand会阻塞 所以这里使用 分步骤
    // 添加命令
    if (redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        // 把命令发过去 但是不等待接收 
        if (redisBufferWrite(this->_subcribe_context, &done) == REDIS_ERR)
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 取消订阅
bool Redis::unsubscribe(int channel)
{
    if (redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (redisBufferWrite(this->_subcribe_context, &done) == REDIS_ERR)
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    // 等待消息
    while (redisGetReply(this->_subcribe_context, (void **)&reply) == REDIS_OK)
    {
        // reply是个size为3的数组 最后一个元素为消息 倒数第二个元素为通道号
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 订阅的用户收到消息后 服务器调用 消息转发服务
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}


// 初始化回调操作
void Redis::init_notify_handler(function<void(int, string)> fn) {
    this->_notify_message_handler = fn;
}
