#include "groupmodel.hpp"
#include "db.h"
#include <sstream>
// #include <iostream>

bool GroupModel::createGroup(Group &group) {
    std::string sql;
    std::stringstream ss;
    ss << "insert into allgroup(groupname, groupdesc) values (\'" << group.getName() << "\', \'" << group.getDesc() << "\')";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

void GroupModel::addGroup(int userid, int groupid, std::string role) {
    std::string sql;
    std::stringstream ss;
    ss << "insert into groupuser values (" << groupid << ", " << userid << ", \'" << role << "\')";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        mysql.update(sql);
    }
}

std::vector<Group> GroupModel::queryGroups(int userid) {
    std::string sql;
    std::stringstream ss;
    ss << "select a.id, a.groupname, a.groupdesc from allgroup a left join groupuser g on a.id = g.groupid where g.userid = " << userid;
    sql = ss.str();
    std::vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) !=nullptr) {
                groupVec.emplace_back(Group(atoi(row[0]), row[1], row[2]));
            }
        }
        mysql_free_result(res);
    }
    for(auto& group : groupVec) {
        ss.str("");
        ss.clear();
        ss << "select u.id, u.name, u.state, g.grouprole from user u join groupuser g on u.id = g.userid where g.groupid = " << group.getId();
        sql = ss.str();
        MYSQL_RES* res = mysql.query(sql);
        // std::cout << "sql-----> " << sql << std::endl;
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) !=nullptr) {
                group.getUsers().emplace_back(GroupUser(atoi(row[0]), row[1], row[2], row[3]));
                // std::cout << "user------>" << atoi(row[0]) << ' ' << row[1] << ' ' << row[2] << ' ' << row[3] << std::endl;
            }
        }
        mysql_free_result(res);
    }
    return groupVec;
}

std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    std::string sql;
    std::stringstream ss;
    ss << "select userid from groupuser where groupid = " << groupid << " and userid != " << userid;
    sql = ss.str();
    std::vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) !=nullptr) {
                idVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}
