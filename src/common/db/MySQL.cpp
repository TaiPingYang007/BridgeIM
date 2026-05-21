#include "../../../include/common/db/MySQL.hpp"

#include <iostream>

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

bool MySQL::connect(std::string ip, unsigned short port,
                    std::string user, std::string password, std::string dbname)
{
    if (mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(),
                           dbname.c_str(), port, nullptr, 0) != nullptr)
    {
        mysql_set_character_set(_conn, "utf8");
        return true;
    }

    std::cerr << "mysql_real_connect failed, host=" << ip << ":" << port
              << " db=" << dbname << " user=" << user
              << " error=" << mysql_error(_conn) << std::endl;

    return false;
}

bool MySQL::update(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        return false;
    }

    return true;
}

MYSQL_RES *MySQL::query(std::string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        return nullptr;
    }

    return mysql_store_result(_conn);
}

MYSQL *MySQL::getConnection()
{
    return _conn;
}
