#ifndef LOGGER_H
#define LOGGER_H

#include "lockqueue.h"
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>

enum class LogLevel {
  INFO,
  ERROR,
};

class Logger {
public:
  static Logger &GetInstance();

  bool Init(const std::string &logDir, const std::string &programName,
            const std::string &instanceTag); // 文件路径 + 程序名称 + 实例标签（端口）
  void Log(const std::string &msg, LogLevel level); // 向日志队列中写入数据
  void Shutdown();                                  // 排空日志队列并关闭消费线程
  std::size_t DroppedCount() const;                 // 队列满时累计丢弃的日志条数

private:
  Logger();
  ~Logger();
  void StartConsumer();
  std::string BuildPrefix(LogLevel level) const;
  bool IsShutdown() const;
  static bool LocalTime(time_t now, tm &out);

  Logger(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger &operator=(Logger &&) = delete;

private:
  std::mutex _configMutex;
  std::string _logDir;      // 记录文件夹路径
  std::string _programName; // 记录程序名称
  std::string _instanceTag; // 记录实例标签，用来记录对应的服务器实例
  bool _initialized;        // 是否已经初始化
  std::atomic<unsigned long> _configEpoch;

  LockQueue<std::string> _lockQueue;
  std::thread _consumer;
  mutable std::mutex _lifecycleMutex;
  bool _shutdown;
};

// muduo/base/Logging.h 也定义了 LOG_INFO / LOG_ERROR（stream 风格），
// 先 undef 再重新定义为 printf 风格，确保我们的版本始终生效。
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define LOG_INFO(logmsgformat, ...)                                            \
  do {                                                                         \
    Logger &logger = Logger::GetInstance();                                    \
    char c[1024] = {0};                                                        \
    std::snprintf(c, sizeof(c), logmsgformat, ##__VA_ARGS__);                  \
    logger.Log(c, LogLevel::INFO);                                             \
  } while (0)

#define LOG_ERROR(logmsgformat, ...)                                           \
  do {                                                                         \
    Logger &logger = Logger::GetInstance();                                    \
    char c[1024] = {0};                                                        \
    std::snprintf(c, sizeof(c), logmsgformat, ##__VA_ARGS__);                  \
    logger.Log(c, LogLevel::ERROR);                                            \
  } while (0)

#endif
