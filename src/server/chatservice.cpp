#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include <iostream>

using namespace muduo;

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService() {
    m_msgHandlerMap.insert(std::make_pair(LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)));
    m_msgHandlerMap.insert(std::make_pair(GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)));

    if(m_redis.connect()) {
        m_redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage, this, _1, _2));
    }
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    std::string name = js["name"];
    std::string pwd = js["password"];
    User user = m_userModel.query(name);
    if(user.getName() == name && user.getPassword() == pwd) {
        if(user.getState() == "online") {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "此用户已经登录了";
            conn->send(response.dump());
        } else {
            {
                std::lock_guard<std::mutex> lock(m_mtxConn);
                m_userConnMap.insert(std::make_pair(user.getId(), conn));
            }
            m_redis.subscribe(user.getId());
            user.setState("online");
            m_userModel.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            auto msgVec = m_offlineMsgModel.query(user.getId());
            if(!msgVec.empty()) {
                response["offlinemsg"] = msgVec;
                m_offlineMsgModel.remove(user.getId());
            }
            auto userVec = m_friendModel.query(user.getId());
            if((!userVec.empty())) {
                std::vector<std::string> tmpVec;
                for(auto& p_user : userVec) {
                    json tmp;
                    tmp["id"] = p_user.getId();
                    tmp["name"] = p_user.getName();
                    tmp["state"] = p_user.getState();
                    tmpVec.push_back(tmp.dump());
                }
                response["friends"] = tmpVec;
            }
            auto groupVec = m_groupModel.queryGroups(user.getId());
            if(!groupVec.empty()) {
                std::vector<std::string> tmpVec;
                for(auto& group : groupVec) {
                    json tmp;
                    tmp["id"] = group.getId();
                    tmp["groupname"] = group.getName();
                    tmp["groupdesc"] = group.getDesc();
                    std::vector<std::string> res;
                    for(auto& p_user : group.getUsers()) {
                        json p_js;
                        p_js["id"] = p_user.getId();
                        p_js["name"] = p_user.getName();
                        p_js["state"] = p_user.getState();
                        p_js["role"] = p_user.getRole();
                        res.push_back(p_js.dump());
                    }
                    tmp["users"] = res;
                    tmpVec.push_back(tmp.dump());
                }
                response["groups"] = tmpVec;
            }
            conn->send(response.dump());
        }
    } else {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    // LOG_INFO << "reg service...";
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = m_userModel.insert(user);
    if(state) {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int p_id = js["to"].get<int>();
    {
        std::lock_guard<std::mutex> lock(m_mtxConn);
        auto it = m_userConnMap.find(p_id);
        if(it != m_userConnMap.end()) {
            it->second->send(js.dump());
            return;
        }
    }
    User user = m_userModel.query(p_id);
    if(user.getState() == "online") {
        m_redis.publish(p_id, js.dump());
        return;
    }
    m_offlineMsgModel.insert(p_id, js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    m_friendModel.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];
    Group group(-1, name, desc);
    if(m_groupModel.createGroup(group)) {
        m_groupModel.addGroup(userid, group.getId(), "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    m_groupModel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    js["name"] = m_userModel.query(userid).getName();
    // std::cout << "groupChat--------> " << js << std::endl;
    std::vector<int> useridVec = m_groupModel.queryGroupUsers(userid, groupid);
    std::lock_guard<std::mutex> lock(m_mtxConn);
    for(auto& id : useridVec) {
        auto it = m_userConnMap.find(id);
        if(it != m_userConnMap.end()) {
            it->second->send(js.dump());
        } else {
            User user = m_userModel.query(userid);
            if(user.getState() == "online") {
                m_redis.publish(id, js.dump());
            } else {
                m_offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"];
    {
        std::lock_guard<std::mutex> lock(m_mtxConn);
        auto it = m_userConnMap.find(userid);
        if(it != m_userConnMap.end()) {
            m_userConnMap.erase(it);
        }
    }
    m_redis.unsubscribe(userid);
    User p_user(userid);
    m_userModel.updateState(p_user);
}

void ChatService::handlerRedisSubscribeMessage(int userid, std::string msg) {
    std::lock_guard<std::mutex> lock(m_mtxConn);
    auto it = m_userConnMap.find(userid);
    if(it != m_userConnMap.end()) {
        it->second->send(msg);
        return;
    }
    m_offlineMsgModel.insert(userid, msg);
}

MsgHandler ChatService::getHandler(int msgid) {
    auto it = m_msgHandlerMap.find(msgid);
    if(it == m_msgHandlerMap.end()) {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    } else {
        return it->second;
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        std::lock_guard<std::mutex> lock(m_mtxConn);
        for(auto& [p_id, p_conn] : m_userConnMap) {
            if(p_conn == conn) {
                user.setId(p_id);
                m_userConnMap.erase(p_id);
                break;
            }
        }
    }
    m_redis.unsubscribe(user.getId());
    if(user.getId() != -1) {
        user.setState("offline");
        m_userModel.updateState(user);
    }
}

void ChatService::reset() {
    m_userModel.resetState();
}
