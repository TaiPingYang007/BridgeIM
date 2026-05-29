#include "../../include/server/chatserver.hpp"
#include "../../include/server/chatservice.hpp"
#include "../../include/server/logger.h"
#include <cstddef>
#include <mutex>

namespace {
ThreadPool::Config makeBusinessThreadPoolConfig() {
  ThreadPool::Config config;
  config.thread_count = 4;
  config.max_queue_size = 10000;
  return config;
}
} // namespace

// 初始化聊天服务系统对象
ChatServer::ChatServer(muduo::net::EventLoop *loop,
                       const muduo::net::InetAddress &listenAddr,
                       const std::string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop),
      _threadPool(makeBusinessThreadPoolConfig()) {
  // 注册连接回调
  _server.setConnectionCallback(
      [this](const muduo::net::TcpConnectionPtr &conn) {
        onConnection(conn);
      });
  // 注册消息回调
  _server.setMessageCallback(
      [this](const muduo::net::TcpConnectionPtr &conn,
             muduo::net::Buffer *buf, muduo::Timestamp readTime) {
        onMessage(conn, buf, readTime);
      });
  // 设置线程数量
  _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start() { _server.start(); }

// 上报连接相关的回调函数
void ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn) {
  // 客户端断开连接
  if (!conn->connected()) {
    try {
      _threadPool.enqueue([conn]() {
        try {
          ChatService::instance()->clientCloseException(conn);
        } catch (const std::exception &e) {
          LOG_ERROR("client close cleanup exception: %s\n", e.what());
        } catch (...) {
          LOG_ERROR("unknown client close cleanup exception\n");
        }
      });
    } catch (const std::exception &e) {
      LOG_ERROR("failed to enqueue client close cleanup task: %s\n", e.what());
      ChatService::instance()->clientCloseException(conn);
    }

    // 释放_recvBufMap中的数据
    {
      std::lock_guard<std::mutex> lock(_recvBufMutex);
      _recvBufMap.erase(conn);
    }

    conn->shutdown();
  }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf, muduo::Timestamp time) {
  // 将接收到的数据转换成字符串，方便后续处理
  std::string readbuf = buf->retrieveAllAsString();

  // 接收到消息后先放到对应的conn的缓存中
  {
    std::lock_guard<std::mutex> lock(_recvBufMutex);
    _recvBufMap[conn] += readbuf;
  }

  size_t pos = 0;
  // 保证线程安全
  {
    std::lock_guard<std::mutex> lock(_recvBufMutex);
    pos = _recvBufMap[conn].find("\n");
  }
  while (pos != std::string::npos) {
    std::string onMessage;
    {
      std::lock_guard<std::mutex> lock(_recvBufMutex);
      onMessage = _recvBufMap[conn].substr(0, pos);
      _recvBufMap[conn].erase(0, pos + 1);
      pos = _recvBufMap[conn].find('\n');
    }

    // 保持代码健壮性，如果提取到的消息是空字符串，就继续循环等待下一条消息
    if (onMessage.empty()) {
      continue;
    }

    try {
      // 数据的反序列化
      nlohmann::json js = nlohmann::json::parse(onMessage);

      // 防范：有没有传 msgid？
      if (!js.contains("msgid")) {
        LOG_ERROR("missing msgid in json: '%s'\n", onMessage.c_str());
        return;
      }
      // 目的是完全解耦网络模块的代码和业务模块的代码
      auto msgHandler =
          ChatService::instance()->getHandler(js["msgid"].get<int>());

      // 将业务处理投递到线程池，避免阻塞 Muduo IO 线程
      try {
        _threadPool.enqueue([conn, js, time, msgHandler]() mutable {
          try {
            msgHandler(conn, js, time);
          } catch (const std::exception &e) {
            LOG_ERROR("business handler exception: %s\n", e.what());
          } catch (...) {
            LOG_ERROR("unknown business handler exception\n");
          }
        });
      } catch (const std::exception &e) {
        LOG_ERROR("failed to enqueue business task: %s\n", e.what());
        nlohmann::json response;
        response["errno"] = 1;
        response["errmsg"] = "server busy";
        conn->send(response.dump() + "\n");
      }

    } catch (const nlohmann::json::parse_error &e) {
      LOG_ERROR("JSON parse error: %s. Data: %s\n", e.what(),
                onMessage.c_str());
    } catch (const nlohmann::json::type_error &e) {
      LOG_ERROR("JSON type conversion error: %s. Data: %s\n", e.what(),
                onMessage.c_str());
    } catch (const nlohmann::json::out_of_range &e) {
      LOG_ERROR("JSON map key not found: %s. Data: %s\n", e.what(),
                onMessage.c_str());
    } catch (...) {
      LOG_ERROR("Unknown exception caught in onMessage.\n");
    }
  }
}
