#pragma once

#include <mysql/mysql.h>
#include <string>

class MySQL {
public:
    MySQL();
    ~MySQL();
    bool connect();
    bool update(std::string sql);
    MYSQL_RES* query(std::string sql);
    MYSQL* getConnection();
private:
    MYSQL* m_conn;
};