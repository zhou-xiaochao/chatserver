#include "offlinemessagemodel.hpp"
#include "db.h"
#include <sstream>

void OfflineMsgModel::insert(int userid, std::string msg) {
    std::string sql;
    std::stringstream ss;
    ss << "insert into offlinemessage(userid, message) values (" << userid << ", \'" << msg << "\')";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            return;
        }
    }
}

void OfflineMsgModel::remove(int userid) {
    std::string sql;
    std::stringstream ss;
    ss << "delete from offlinemessage where userid = " << userid;
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            return;
        }
    }
}

std::vector<std::string> OfflineMsgModel::query(int userid)
{
    std::string sql;
    std::stringstream ss;
    ss << "select message from offlinemessage where userid = " << userid;
    sql = ss.str();
    std::vector<std::string> vec;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) !=nullptr) {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
