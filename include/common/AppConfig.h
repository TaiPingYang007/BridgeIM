#ifndef BRIDGEIM_APPCONFIG_H
#define BRIDGEIM_APPCONFIG_H

#include <string>
#include <cstdint>

class AppConfig
{
public:
    struct ServerConfig
    {
        std::string listen_ip = "127.0.0.1";
        unsigned short listen_port = 6000;
        int worker_threads = 4;
        std::string log_dir = "./bin";
        // 最大网络数据包大小，单位字节
        std::size_t max_packet_size = 65536;
    };

    struct MySqlConfig
    {
        std::string host = "127.0.0.1";
        unsigned short port = 3306;
        std::string username;
        std::string password;
        std::string database;
        std::string charset = "gbk";
        int pool_init_size = 4;
        int pool_max_size = 16;
        int pool_connection_timeout_ms = 1000;
    };

    struct RedisConfig
    {
        std::string host = "127.0.0.1";
        uint16_t port = 6379;
        std::string password;
    };

    struct ThreadPoolConfig
    {
        bool enabled = false;
        std::size_t thread_count = 4;
        std::size_t max_queue_size = 10000;
    };

public:
    static AppConfig &instance();

    // 加载配置文件，返回是否成功
    bool LoadFromFile(const std::string &configDir);

    const ServerConfig &GetServerConfig() const;
    const MySqlConfig &GetMySqlConfig() const;
    const RedisConfig &GetRedisConfig() const;
    const ThreadPoolConfig &GetThreadPoolConfig() const;

    const std::string &GetLastError() const;

private:
    AppConfig() = default;
    
    AppConfig &operator=(const AppConfig &) = delete;
    AppConfig(const AppConfig &) = delete;
    AppConfig(AppConfig &&) = delete;
    AppConfig &operator=(AppConfig &&) = delete;

private:
    ServerConfig server_config_;
    MySqlConfig mysql_config_;
    RedisConfig redis_config_;
    ThreadPoolConfig thread_pool_config_;
    std::string last_error_;
};

#endif // BRIDGEIM_APPCONFIG_H