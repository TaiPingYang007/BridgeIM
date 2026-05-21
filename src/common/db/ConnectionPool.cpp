#include "../../../include/common/db/ConnectionPool.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool{};
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    if (!_config.loadConfig("./config/connection_pool.conf"))
    {
        std::cerr << "连接池配置加载失败，程序退出！" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const PoolConfig &poolConfig = _config.getPoolConfig();

    for (int i = 0; i < poolConfig.initSize; ++i)
    {
        MySQL *conn = createConnection();
        if (conn == nullptr)
        {
            const DbConfig &dbConfig = _config.getDbConfig();
            std::cerr << "MySQL 初始化连接失败，程序退出！ host="
                      << dbConfig.ip << ":" << dbConfig.port
                      << " db=" << dbConfig.dbname
                      << " user=" << dbConfig.username << std::endl;
            std::exit(EXIT_FAILURE);
        }

        _connectionQueue.push(conn);
        ++_connectionCnt;
    }

    _isRunning = true;
    _produceThread = std::thread([this]() { produceConnectionTask(); });
}

ConnectionPool::~ConnectionPool()
{
    _isRunning = false;
    _cvProducer.notify_all();
    _cvConsumer.notify_all();

    if (_produceThread.joinable())
    {
        _produceThread.join();
    }

    std::unique_lock<std::mutex> lock(_queueMutex);
    _cvShutdown.wait(lock, [this]() { return _borrowedCnt == 0; });

    while (!_connectionQueue.empty())
    {
        MySQL *conn = _connectionQueue.front();
        _connectionQueue.pop();
        delete conn;
    }
}

std::shared_ptr<MySQL> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    const PoolConfig &poolConfig = _config.getPoolConfig();

    bool ok = _cvConsumer.wait_for(
        lock,
        std::chrono::milliseconds(poolConfig.connectionTimeout),
        [this]()
        {
            return !_connectionQueue.empty() || !_isRunning;
        });

    if (!ok)
    {
        return nullptr;
    }

    if (!_isRunning)
    {
        return nullptr;
    }

    MySQL *conn = _connectionQueue.front();
    _connectionQueue.pop();
    ++_borrowedCnt;

    if (static_cast<int>(_connectionQueue.size()) < poolConfig.initSize)
    {
        _cvProducer.notify_one();
    }

    std::shared_ptr<MySQL> sp(conn, [this](MySQL *ptr)
                              {
                                  std::unique_lock<std::mutex> lock(_queueMutex);

                                  if (_isRunning)
                                  {
                                      _connectionQueue.push(ptr);
                                      _cvConsumer.notify_one();
                                  }
                                  else
                                  {
                                      delete ptr;
                                  }

                                  --_borrowedCnt;
                                  if (_borrowedCnt == 0)
                                  {
                                      _cvShutdown.notify_one();
                                  } });

    return sp;
}

void ConnectionPool::produceConnectionTask()
{
    const PoolConfig &poolConfig = _config.getPoolConfig();

    while (_isRunning)
    {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            while ((_connectionQueue.size() >= static_cast<size_t>(poolConfig.initSize) ||
                    _connectionCnt >= poolConfig.maxSize) &&
                   _isRunning)
            {
                _cvProducer.wait(lock);
            }

            if (!_isRunning)
            {
                break;
            }
        }

        MySQL *conn = createConnection();
        if (conn == nullptr)
        {
            continue;
        }

        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            if (!_isRunning)
            {
                delete conn;
                break;
            }

            if (_connectionQueue.size() >= static_cast<size_t>(poolConfig.initSize) ||
                _connectionCnt >= poolConfig.maxSize)
            {
                delete conn;
                continue;
            }

            _connectionQueue.push(conn);
            ++_connectionCnt;
        }

        _cvConsumer.notify_one();
    }
}

MySQL *ConnectionPool::createConnection()
{
    const DbConfig &dbConfig = _config.getDbConfig();

    MySQL *conn = new MySQL();
    if (conn->connect(dbConfig.ip,
                      dbConfig.port,
                      dbConfig.username,
                      dbConfig.password,
                      dbConfig.dbname))
    {
        return conn;
    }

    delete conn;
    return nullptr;
}
