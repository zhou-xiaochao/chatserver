#include "usermodel.hpp"
#include "db.h"
#include <sstream>

bool UserModel::insert(User &user) {
    std::string sql;
    std::stringstream ss;
    ss << "insert into user(name, password, state) values (\'" << user.getName() << "\', \'" << user.getPassword() << "\', \'" << user.getState() << "\')";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::query(std::string name)
{
    std::string sql;
    std::stringstream ss;
    ss << "select * from user where name = \'" << name << "\'";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

User UserModel::query(int id)
{
    std::string sql;
    std::stringstream ss;
    ss << "select * from user where id = \'" << id << "\'";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

bool UserModel::updateState(User &user)
{
    std::string sql;
    std::stringstream ss;
    ss << "update user set state = \'" << user.getState() << "\' where id = \'" << user.getId() << "\'";
    sql = ss.str();
    MySQL mysql;
    if(mysql.connect()) {
        if(mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

void UserModel::resetState() {
    std::string sql = "update user set state = \'offline\' where state = \'online\'";
    MySQL mysql;
    if(mysql.connect()) {
        mysql.update(sql);
    }
}
