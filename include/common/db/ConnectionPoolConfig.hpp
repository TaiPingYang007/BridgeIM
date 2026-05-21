#pragma once

#include <string>

// 数据库的连接配置
struct DbConfig
{
    std::string ip;
    unsigned short port;
    std::string username;
    std::string password;
    std::string dbname;
};

// 连接池运行配置
struct PoolConfig
{
    int initSize = 0;
    int maxSize = 0;
    int connectionTimeout = 0; // 获取连接的最大等待时间，单位：毫秒
};

class ConnectionPoolConfig
{
public:
    // 从配置文件中加载配置
    bool loadConfig(const std::string &path);

    // 获取数据库配置
    const DbConfig &getDbConfig() const;

    // 获取连接池配置
    const PoolConfig &getPoolConfig() const;

private:
    // 给不同的 key 赋值，路由分发
    bool assignConfigItem(const std::string &key, const std::string &value);

    // 当配置文件不存在时，从环境变量回填数据库配置和池默认值
    void loadFromEnvironment();

    // 检查配置是否合法
    bool validate() const;

    // 去掉字符串首尾空白
    static std::string trim(const std::string &text);

    // 把字符串解析成 int
    static bool parseInt(const std::string &text, int &value);

private:
    DbConfig _dbConfig;
    PoolConfig _poolConfig;

    // 这些标记位用于区分“字段缺失”和“字段存在但为空/为 0”
    bool _hasIp = false;
    bool _hasPort = false;
    bool _hasUsername = false;
    bool _hasPassword = false;
    bool _hasDbName = false;
    bool _hasInitSize = false;
    bool _hasMaxSize = false;
    bool _hasConnectionTimeout = false;
};
