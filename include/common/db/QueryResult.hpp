#pragma once

#include <string>
#include <mysql/mysql.h>

class QueryResult
{
public:
    // 构造函数，初始化查询结果集和状态标志
    QueryResult(MYSQL_RES *result, bool valid);

    // 析构函数，释放查询结果集资源
    ~QueryResult();

    // 禁止复制构造和赋值操作
    QueryResult(const QueryResult &) = delete;
    QueryResult &operator=(const QueryResult &) = delete;
    
    // 允许移动构造和移动赋值操作
    QueryResult(QueryResult &&) noexcept;
    QueryResult &operator=(QueryResult &&) noexcept;

    // 获取查询结果集状态
    bool valid() const;

    // 获取查询结果集当前行的整数值
    int getInt(int index) const;

    // 获取查询结果集当前行的字符串值
    std::string getString(int index) const;

    // 获取查询结果集当前行的下一行数据，如果不存在返回false
    bool next();

private:
    // 查询结果集
    MYSQL_RES *_result;

    // 查询结果集当前行
    MYSQL_ROW _currentRow;

    // 查询结果集状态标志
    bool _valid;
};