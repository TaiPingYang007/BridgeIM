#include "../../../include/common/db/DbSession.hpp"
#include "../../../include/common/db/QueryResult.hpp"
#include "../../../include/common/db/ConnectionPool.hpp"
#include "../../../include/common/db/MySQL.hpp"

#include <mysql/mysql.h>

// 默认构造函数
DbSession::DbSession()
{
    // 从连接池获取一个连接对象
    _conn = ConnectionPool::getConnectionPool()->getConnection();
}

// 执行SQL更新，返回是否成功
bool DbSession::update(const std::string &sql)
{
    if (!_conn)
    {
        // 无法获取连接，更新失败
        return false;
    }

    // 实现更新逻辑
    return _conn->update(sql);
}

// 执行SQL查询，返回查询结果对象
QueryResult DbSession::query(const std::string &sql)
{
    if (!_conn)
    {
        // 无法获取连接，查询失败
        return QueryResult{nullptr, false};
    }

    // 实现查询逻辑
    MYSQL_RES *result = _conn->query(sql);

    if (!result)
    {
        // 查询失败，返回一个无效的查询结果对象
        return QueryResult{result, false};
    }

    return QueryResult{result, true};
}

// 获取最后插入的ID
long long DbSession::lastInsertId() const
{
    if (!_conn)
    {
        return -1;
    }

    // 实现获取最后插入ID的逻辑
    return mysql_insert_id(_conn->getConnection());
}
