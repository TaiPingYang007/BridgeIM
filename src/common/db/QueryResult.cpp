#include "../../../include/common/db/QueryResult.hpp"
#include <cstdlib>

// 构造函数，初始化查询结果集和状态标志
QueryResult::QueryResult(MYSQL_RES *result, bool valid)
{
    _result = result;
    _valid = valid;
    _currentRow = nullptr;
}

// 析构函数，释放查询结果集资源
QueryResult::~QueryResult()
{
    if (_result)
    {
        mysql_free_result(_result);
    }
}

// 允许移动构造和移动赋值操作
QueryResult::QueryResult(QueryResult && other) noexcept
{
    this->_result = other._result;
    this->_valid = other._valid;
    this->_currentRow = other._currentRow;

    other._result = nullptr;
    other._valid = false;
    other._currentRow = nullptr;
}

QueryResult &QueryResult::operator=(QueryResult &&other) noexcept
{
    if (this != &other)
    {
        if (this->_result)
        {
            mysql_free_result(this->_result);
        }

        this->_result = other._result;
        this->_valid = other._valid;
        this->_currentRow = other._currentRow;

        other._result = nullptr;
        other._valid = false;
        other._currentRow = nullptr;
    }
    return *this;
}

// 获取查询结果集状态
bool QueryResult::valid() const
{
    return _valid;
}

// 获取查询结果集当前行的整数值
int QueryResult::getInt(int index) const
{
    if (!_currentRow || index < 0)
    {
        // 当前行无效或索引无效，返回0
        return 0;
    }

    // 获取当前行指定索引的字符串值
    const char *value = _currentRow[index];

    if (!value)
    {
        // 值为NULL，返回0
        return 0;
    }

    // 将字符串转换为整数并返回
    return std::atoi(value);
}

// 获取查询结果集当前行的字符串值
std::string QueryResult::getString(int index) const
{
    if (!_currentRow || index < 0)
    {
        // 当前行无效或索引无效，返回空字符串
        return "";
    }

    // 获取当前行指定索引的字符串值
    const char *value = _currentRow[index];

    if (!value)
    {
        // 值为NULL，返回空字符串
        return "";
    }

    // 返回字符串值
    return std::string(value);
}

// 获取查询结果集当前行的下一行数据，如果不存在返回false
bool QueryResult::next()
{
    if (!_result)
    {
        return false;
    }

    _currentRow = mysql_fetch_row(_result);
    return !!_currentRow;
}
