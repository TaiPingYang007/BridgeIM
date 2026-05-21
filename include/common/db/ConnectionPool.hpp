#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "ConnectionPoolConfig.hpp"
#include "MySQL.hpp"

class ConnectionPool
{
public:
    // 获取连接池单例
    static ConnectionPool *getConnectionPool();

    // 业务线程借连接
    std::shared_ptr<MySQL> getConnection();

private:
    ConnectionPool();
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    // 后台补充连接线程
    void produceConnectionTask();

    // 按配置创建一个可用连接
    MySQL *createConnection();

private:
    // 空闲连接队列
    std::queue<MySQL *> _connectionQueue;

    // 保护队列线程安全
    std::mutex _queueMutex;

    // 生产者 / 消费者 / 析构收尾同步
    std::condition_variable _cvProducer;
    std::condition_variable _cvConsumer;
    std::condition_variable _cvShutdown;

    // 当前借出连接数
    int _borrowedCnt = 0;

    // 连接池运行状态
    std::atomic<bool> _isRunning{false};

    // 后台补连接线程
    std::thread _produceThread;

    // 配置对象
    ConnectionPoolConfig _config;

    // 当前总连接数
    int _connectionCnt = 0;
};
