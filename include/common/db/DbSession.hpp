#pragma once

#include <memory>
#include <string>

// 前向声明，避免包含完整的头文件？
class MySQL;
class QueryResult;

class DbSession
{
public:
    // 默认构造函数
    DbSession();

    // 禁止复制构造和赋值操作
    DbSession(const DbSession &) = delete;
    DbSession &operator=(const DbSession &) = delete;
    DbSession(DbSession &&) noexcept = default;
    DbSession &operator=(DbSession &&) noexcept = default;

    // 执行SQL更新，返回是否成功
    bool update(const std::string &sql);

    // 执行SQL查询，返回查询结果对象
    QueryResult query(const std::string &sql);

    // 获取最后插入的ID
    long long lastInsertId() const;

private:
    // 从连接池借来的底层连接对象
    std::shared_ptr<MySQL> _conn;
};
