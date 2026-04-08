#pragma once

#include <muduo/net/TcpServer.h>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;

class ChatService {
public:
    static ChatService* instance();
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);
    void handlerRedisSubscribeMessage(int, std::string);
    MsgHandler getHandler(int msgid);
    void clientCloseException(const TcpConnectionPtr& conn);
    void reset();
private:
    ChatService();
    std::unordered_map<int, MsgHandler> m_msgHandlerMap;
    std::unordered_map<int, TcpConnectionPtr> m_userConnMap;
    std::mutex m_mtxConn;
    UserModel m_userModel;
    OfflineMsgModel m_offlineMsgModel;
    FriendModel m_friendModel;
    GroupModel m_groupModel;
    Redis m_redis;
};