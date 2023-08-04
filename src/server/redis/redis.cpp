#include "redis.hpp"
#include <iostream>

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
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success!" << endl;
    return true;
}

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
    if (redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel) == REDIS_ERR)
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
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
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 订阅的用户收到消息后 服务器调用 消息转发服务
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn) {
    this->_notify_message_handler = fn;
}
