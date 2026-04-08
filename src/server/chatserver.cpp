#include "chatserver.hpp"
#include "chatservice.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <thread>
#include <string>

using namespace std::placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg)
: m_server(loop, listenAddr, nameArg), m_loop(loop) {
    m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    m_server.setThreadNum(std::thread::hardware_concurrency());
}

void ChatServer::start() {
    m_server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    if(!conn->connected()) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time) {
    std::string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    ChatService::instance()->getHandler(js["msgid"].get<int>())(conn, js, time);
}
