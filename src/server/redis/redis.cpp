#include <redis.hpp>
#include <iostream>

Redis::Redis()
    : m_publish_context(nullptr), m_subscribe_context(nullptr) {
}

Redis::~Redis() {
    if(m_publish_context != nullptr) {
        redisFree(m_publish_context);
    }
    if(m_subscribe_context != nullptr) {
        redisFree(m_subscribe_context);
    }
}

bool Redis::connect() {
    m_publish_context = redisConnect("127.0.0.1", 6379);
    if(m_publish_context == nullptr) {
        std::cerr << "connect redis failed!\n";
        return false;
    }
    m_subscribe_context = redisConnect("127.0.0.1", 6379);
    if(m_subscribe_context == nullptr) {
        std::cerr << "connect redis failed!\n";
        return false;
    }
    std::thread t([&]() {
        observer_channel_message();
    });
    t.detach();
    std::cout << "connect redis-server success!" << std::endl;
    return true;
}

bool Redis::publish(int channel, std::string message) {
    // std::cout << channel << "------publist----> " << message << '\n';
    redisReply* replay = (redisReply*)redisCommand(m_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(replay == nullptr) {
        std::cerr << "publish command failed!\n";
        return false;
    }
    freeReplyObject(replay);
    return true;
}

bool Redis::subscribe(int channel) {
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!\n";
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done))
        {
            std::cerr << "subscribe command failed!\n";
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->m_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!\n";
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->m_subscribe_context, &done))
        {
            std::cerr << "unsubscribe command failed!\n";
            return false;
        }
    }
    return true;
}

void Redis::observer_channel_message() {
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->m_subscribe_context, reinterpret_cast<void **>(&reply)))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            //
            m_notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    std::cerr << ">>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<" << std::endl;
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn) {
    this->m_notify_message_handler = fn;
}
