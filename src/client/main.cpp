#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
#include <unordered_map>
#include <functional>

using json = nlohmann::json;

User g_currentUser;
std::vector<User> g_currentUserFriendList;
std::vector<Group> g_currentUserGroupList;

bool is_login = false;

void showCurrentUserData();

void readTaskHandler(int clientfd);

std::string getCurrentTime();

void mainMenu(int clientfd);

int main(int argc, char **argv) {
    if(argc < 3) {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000\n";
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1) {
        std::cerr << "socket create error!\n";
        exit(-1);
    }
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    if(connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in))) {
        std::cerr << "connect server error\n";
        close(clientfd);
        exit(-1);
    }
    while(true) {
        g_currentUserFriendList.clear();
        g_currentUserGroupList.clear();
        std::cout << "============================\n";
        std::cout << "1.login\n";
        std::cout << "2.register\n";
        std::cout << "3.quit\n";
        std::cout << "============================\n";
        std::cout << "choice:";
        int choice = 0;
        std::string input;
        std::getline(std::cin, input); // 永远不会因为类型不匹配而阻塞
        try {
            choice = stoi(input); // 尝试将字符串转为整数
        } catch (...) {
            choice = -1; // 转换失败（输入了字母），赋值为一个无效选项
        }
        switch(choice) {
            case 1: {
                std::cout << "+++++++Login+++++++\n";
                std::string name;
                std::string pwd;
                std::cout << "username:";
                std::getline(std::cin, name);
                std::cout << "userpassword:";
                std::getline(std::cin, pwd);
                json js;
                js["msgid"] = LOGIN_MSG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1) {
                    std::cerr << "send login msg error: " << request << '\n';
                } else {
                    char recvbuf[1024] = {0};
                    len = recv(clientfd, recvbuf, 1023, 0);
                    if(len == -1) {
                        std::cerr << "recv login response error\n";
                    } else {
                        json responsejs = json::parse(recvbuf);
                        if(responsejs["errno"].get<int>() != 0) {
                            std::cerr << responsejs["errmsg"] << '\n';
                        } else {
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);
                            if(responsejs.contains("friends")) {
                                std::vector<std::string> friends_vec = responsejs["friends"];
                                for(auto& str : friends_vec) {
                                    json tmp = json::parse(str);
                                    g_currentUserFriendList.emplace_back(User(tmp["id"].get<int>(), tmp["name"], "", tmp["state"]));
                                }
                            }
                            if(responsejs.contains("groups")) {
                                std::vector<std::string> groups_vec = responsejs["groups"];
                                for(auto& str : groups_vec) {
                                    json tmp = json::parse(str);
                                    // std::cout << "groups:----->\n" << tmp << "\n<-------group\n";
                                    Group group(tmp["id"].get<int>(), tmp["groupname"], tmp["groupdesc"]);
                                    std::vector<std::string> users_vec = tmp["users"];
                                    for(auto user_str : users_vec) {
                                        json user_tmp = json::parse(user_str);
                                        group.getUsers().emplace_back(GroupUser(user_tmp["id"].get<int>(), user_tmp["name"], user_tmp["state"], user_tmp["role"]));
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            showCurrentUserData();
                            if(responsejs.contains("offlinemsg")) {
                                std::vector<std::string> off_vec = responsejs["offlinemsg"];
                                for(auto& off_str : off_vec) {
                                    json tmp = json::parse(off_str);
                                    int msgtype = tmp["msgid"].get<int>();
                                    if(msgtype == ONE_CHAT_MSG) {
                                        // std::cout << "one" << std::endl;
                                        std::cout << tmp["time"].get<std::string>() << " [" << tmp["id"] << "] " << tmp["name"].get<std::string>() << " said: " << tmp["msg"].get<std::string>() << std::endl;
                                        continue;
                                    } else if(msgtype == GROUP_CHAT_MSG) {
                                        // std::cout << "group" << std::endl;
                                        std::cout << "群[ " << tmp["groupid"] << " ]消息--> " << tmp["time"].get<std::string>() 
                                        << " [" << tmp["id"] << "] " << tmp["name"].get<std::string>() << " said: " 
                                        << tmp["msg"].get<std::string>() << std::endl;
                                        continue;
                                    }
                                }
                            }
                            static int readthreadnum = 0;
                            if(readthreadnum == 0) {
                                std::thread readTask(readTaskHandler, clientfd);
                                readTask.detach();
                                readthreadnum ++;
                            }
                            is_login = true;
                            mainMenu(clientfd);
                        }
                    }
                }
                break;
            }
            case 2: {
                std::cout << "+++++++Register+++++++\n";
                std::string name;
                std::string pwd;
                std::cout << "username:";
                std::getline(std::cin, name);
                std::cout << "userpassword:";
                std::getline(std::cin, pwd);
                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1) {
                    std::cerr << "send reg msg error: " << request << '\n';
                } else {
                    char recvbuf[1024] = {0};
                    len = recv(clientfd, recvbuf, 1023, 0);
                    if(len == -1) {
                        std::cerr << "recv reg response error\n";
                    } else {
                        json responsejs = json::parse(recvbuf);
                        if(responsejs["errno"].get<int>() != 0) {
                            std::cerr << name << " is already exist\n";
                        } else {
                            std::cout << name << " register success, userid is " << responsejs["id"] << '\n';
                        }
                    }
                }
                break;
            }
            case 3: {
                close(clientfd);
                exit(0);
            }
            default: {
                std::cerr << "invalid input!\n";
                break;
            }
        }
    }
}

void showCurrentUserData() {
    std::cout << "====================login user====================" << std::endl;
    std::cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << std::endl;
    std::cout << "--------------------friend list--------------------" << std::endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "--------------------group list--------------------" << std::endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            // std::cout << "--------------------groupuser list--------------------" << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                          << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "================================================" << std::endl;
}

void readTaskHandler(int clientfd) {
    while(true) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1023, 0);
        if(len == -1 || len == 0) {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(msgtype == ONE_CHAT_MSG) {
            std::cout << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        } else if(msgtype == GROUP_CHAT_MSG) {
            std::cout << "群[ " << js["groupid"] << " ]消息--> " << js["time"].get<std::string>() << " [" << js["id"] << "] " << js["name"].get<std::string>() << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
    }
}

std::string getCurrentTime() {
    // 获取当前系统时间点
    auto now = std::chrono::system_clock::now();
    // 转换为time_t（日历时间）
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    // 转换为本地时间（线程安全版本，C++11+）
    std::tm tm_info;
    localtime_r(&now_time, &tm_info);  // Linux/macOS用这个
    // Windows请用：localtime_s(&tm_info, &now_time);

    // 格式化输出：年-月-日 时:分:秒
    std::ostringstream oss;
    oss << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "logout" command handler
void logout(int, std::string);

std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"logout", "注销, 格式logout"}
};

std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}
};

void mainMenu(int clientfd) {
    help();
    std::string buffer;
    while(is_login) {
        std::getline(std::cin, buffer);
        std::string commandbuf(buffer);
        std::string command;
        int idx = commandbuf.find(":");
        if(idx == -1) {
            command = commandbuf;
        } else {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()) {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int fd, std::string str) {
    std::cout << "show command list >>> " << std::endl;
    for(auto& p : commandMap) {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

void chat(int clientfd, std::string str) {\
    int idx = str.find(":");
    if(idx == -1) {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }
    int friendid = 0;
    try {
        friendid = atoi(str.substr(0, idx).c_str());
    } catch(...) {
        std::cerr << "chat command error!\n";
        return;
    }
    std::string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}

void addfriend(int clientfd, std::string str) {
    int friendid = 0;
    try {
        friendid = atoi(str.c_str());
    } catch(...) {
        std::cerr << "addfriend command error!\n";
        return;
    }
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}

void creategroup(int clienfd, std::string str) {
    int idx = str.find(":");
    if(idx == -1) {
        std::cerr << "creategroup command errror!" << std::endl;
        return;
    }
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();
    int len = send(clienfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}

void addgroup(int clientfd, std::string str) {
    int groupid = 0;
    try {
        groupid = atoi(str.c_str()); // 尝试将字符串转为整数
    } catch (...) {
        std::cerr << "addgroup command error!\n"; // 转换失败（输入了字母），赋值为一个无效选项
        return;
    }
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}

void groupchat(int clientfd, std::string str) {
    int idx = str.find(":");
    if(idx == -1) {
        std::cerr << "groupchat command invalid!\n";
    }
    int groupid = 0;
    try {
        groupid = atoi(str.substr(0, idx).c_str()); // 尝试将字符串转为整数
    } catch (...) {
        std::cerr << "addgroup command error!\n"; // 转换失败（输入了字母），赋值为一个无效选项
        return;
    }
    std::string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}

void logout(int clientfd, std::string str) {
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    std::string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), buffer.size() + 1, 0);
    if(len == -1) {
        std::cerr << "send logout msg error -> " << buffer << std::endl;
        return;
    }
    is_login = false;
}
