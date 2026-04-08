#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

class Redis {
public:
    Redis();
    ~Redis();
    //连接redis服务器
    bool connect();
    //向redis指定的通道发消息
    bool publish(int channel, std::string message);
    //向redis指定的通道订阅消息
    bool subscribe(int channel);
    //向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);
    //在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    //初始化向业务层上报通道消息的回调对象
    void init_notify_handler(std::function<void(int, std::string)> fn);
private:
    redisContext* m_publish_context;
    redisContext* m_subscribe_context;
    std::function<void(int, std::string)> m_notify_message_handler;
};