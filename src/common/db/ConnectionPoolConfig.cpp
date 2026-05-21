#include "../../../include/common/db/ConnectionPoolConfig.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

bool ConnectionPoolConfig::loadConfig(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        loadFromEnvironment();
        return validate();
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(ifs, line))
    {
        ++lineNumber;
        std::string trimmedLine = trim(line);

        if (trimmedLine.empty() || trimmedLine[0] == '#')
        {
            continue;
        }

        std::size_t pos = trimmedLine.find('=');
        if (pos == std::string::npos)
        {
            std::cerr << "配置文件格式错误，第 " << lineNumber
                      << " 行缺少 '=' : " << trimmedLine << std::endl;
            return false;
        }

        std::string key = trim(trimmedLine.substr(0, pos));
        std::string value = trim(trimmedLine.substr(pos + 1));

        if (key.empty())
        {
            std::cerr << "配置文件格式错误，第 " << lineNumber
                      << " 行 key 为空" << std::endl;
            return false;
        }

        if (!assignConfigItem(key, value))
        {
            std::cerr << "配置项非法，第 " << lineNumber
                      << " 行: " << trimmedLine << std::endl;
            return false;
        }
    }

    return validate();
}

void ConnectionPoolConfig::loadFromEnvironment()
{
    auto getEnvOrDefault = [](const char *name, const char *defaultValue) {
        const char *value = std::getenv(name);
        if (value == nullptr || value[0] == '\0')
        {
            return std::string(defaultValue);
        }
        return std::string(value);
    };

    _dbConfig.ip = getEnvOrDefault("MYSQL_HOST", "127.0.0.1");
    _dbConfig.username = getEnvOrDefault("MYSQL_USER", "chatserver_app");
    _dbConfig.password = getEnvOrDefault("MYSQL_PASSWORD", "chatserver_dev_password");
    _dbConfig.dbname = getEnvOrDefault("MYSQL_DATABASE", "chatserver");
    _hasIp = true;
    _hasUsername = true;
    _hasPassword = true;
    _hasDbName = true;

    int port = 3306;
    const std::string portText = getEnvOrDefault("MYSQL_PORT", "3306");
    if (parseInt(portText, port) && port > 0 && port <= 65535)
    {
        _dbConfig.port = static_cast<unsigned short>(port);
        _hasPort = true;
    }

    // 连接池参数第一版先给稳定默认值，后续再决定是否提升到显式环境变量
    _poolConfig.initSize = 10;
    _poolConfig.maxSize = 1024;
    _poolConfig.connectionTimeout = 1000;
    _hasInitSize = true;
    _hasMaxSize = true;
    _hasConnectionTimeout = true;
}

const DbConfig &ConnectionPoolConfig::getDbConfig() const { return _dbConfig; }

const PoolConfig &ConnectionPoolConfig::getPoolConfig() const { return _poolConfig; }

bool ConnectionPoolConfig::assignConfigItem(const std::string &key, const std::string &value)
{
    if (key == "ip")
    {
        _dbConfig.ip = value;
        _hasIp = true;
        return true;
    }

    if (key == "port")
    {
        int port = 0;
        if (!parseInt(value, port) || port <= 0 || port > 65535)
        {
            std::cerr << "无效的 port 值: " << value << std::endl;
            return false;
        }

        _dbConfig.port = static_cast<unsigned short>(port);
        _hasPort = true;
        return true;
    }

    if (key == "username")
    {
        _dbConfig.username = value;
        _hasUsername = true;
        return true;
    }

    if (key == "password")
    {
        _dbConfig.password = value;
        _hasPassword = true;
        return true;
    }

    if (key == "dbname")
    {
        _dbConfig.dbname = value;
        _hasDbName = true;
        return true;
    }

    if (key == "initSize")
    {
        int initSize = 0;
        if (!parseInt(value, initSize) || initSize <= 0)
        {
            std::cerr << "无效的 initSize 值: " << value << std::endl;
            return false;
        }

        _poolConfig.initSize = initSize;
        _hasInitSize = true;
        return true;
    }

    if (key == "maxSize")
    {
        int maxSize = 0;
        if (!parseInt(value, maxSize) || maxSize <= 0)
        {
            std::cerr << "无效的 maxSize 值: " << value << std::endl;
            return false;
        }

        _poolConfig.maxSize = maxSize;
        _hasMaxSize = true;
        return true;
    }

    if (key == "connectionTimeout")
    {
        int timeout = 0;
        if (!parseInt(value, timeout) || timeout <= 0)
        {
            return false;
        }

        _poolConfig.connectionTimeout = timeout;
        _hasConnectionTimeout = true;
        return true;
    }

    return false;
}

bool ConnectionPoolConfig::validate() const
{
    if (!_hasIp || _dbConfig.ip.empty())
    {
        std::cerr << "缺少合法配置项: ip" << std::endl;
        return false;
    }

    if (!_hasPort)
    {
        std::cerr << "缺少合法配置项: port" << std::endl;
        return false;
    }

    if (!_hasUsername || _dbConfig.username.empty())
    {
        std::cerr << "缺少合法配置项: username" << std::endl;
        return false;
    }

    if (!_hasPassword)
    {
        std::cerr << "缺少配置项: password" << std::endl;
        return false;
    }

    if (!_hasDbName || _dbConfig.dbname.empty())
    {
        std::cerr << "缺少合法配置项: dbname" << std::endl;
        return false;
    }

    if (!_hasInitSize || _poolConfig.initSize <= 0)
    {
        std::cerr << "缺少合法配置项: initSize" << std::endl;
        return false;
    }

    if (!_hasMaxSize || _poolConfig.maxSize < _poolConfig.initSize)
    {
        std::cerr << "缺少合法配置项或 maxSize < initSize" << std::endl;
        return false;
    }

    if (!_hasConnectionTimeout || _poolConfig.connectionTimeout <= 0)
    {
        std::cerr << "缺少合法配置项: connectionTimeout" << std::endl;
        return false;
    }

    return true;
}

std::string ConnectionPoolConfig::trim(const std::string &text)
{
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
    {
        return "";
    }

    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool ConnectionPoolConfig::parseInt(const std::string &text, int &value)
{
    try
    {
        std::size_t pos = 0;
        int number = std::stoi(text, &pos);
        if (pos != text.size())
        {
            return false;
        }

        value = number;
        return true;
    }
    catch (...)
    {
        return false;
    }
}
