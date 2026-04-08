#include "friendmodel.hpp"
#include "db.h"
#include <sstream>

void FriendModel::insert(int userid, int friendid) {
    std::string user_sql, friend_sql;
    std::stringstream ss;
    ss << "insert into friend values (" << userid << ", " << friendid << ")";
    user_sql = ss.str();
    ss.str("");
    ss.clear();
    ss << "insert into friend values (" << friendid << ", " << userid << ")";
    friend_sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        mysql.update(user_sql);
        mysql.update(friend_sql);
    }
}

std::vector<User> FriendModel::query(int userid) {
    std::string sql;
    std::stringstream ss;
    ss << "select u.id, u.name, u.state from user u left join friend f on f.friendid = u.id where f.userid =  " << userid;
    sql = ss.str();
    std::vector<User> vec;  
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) !=nullptr) {
                vec.emplace_back(User(atoi(row[0]), row[1], "", row[2]));
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
