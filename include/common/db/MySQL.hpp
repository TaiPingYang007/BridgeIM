#pragma once

#include <mysql/mysql.h>
#include <string>

class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
    // 释放数据库连接资源
    ~MySQL();

    // 数据库连接
    bool connect(std::string ip, unsigned short port,
                 std::string user, std::string password, std::string dbname);

    // 更新数据库
    bool update(std::string sql);

    // 查询数据库
    MYSQL_RES *query(std::string sql);

    // 获取底层连接句柄，供 session 查询 lastInsertId 使用
    MYSQL *getConnection();

private:
    // 数据库连接句柄
    MYSQL *_conn;
};
