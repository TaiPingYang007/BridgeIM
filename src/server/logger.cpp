#include "server/logger.h"

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <thread>

namespace {
const std::size_t kDefaultLogQueueCapacity = 10000;

std::size_t LoadQueueCapacity() {
  const char *raw = std::getenv("CHATSERVER_LOG_QUEUE_CAP");
  if (raw == nullptr || *raw == '\0') {
    return kDefaultLogQueueCapacity;
  }

  char *end = nullptr;
  const unsigned long value = std::strtoul(raw, &end, 10);
  if (end == raw || *end != '\0') {
    return kDefaultLogQueueCapacity;
  }
  return static_cast<std::size_t>(value);
}

// EnsureDirectory 确保文件夹存在，如果不存在就创建新文件夹
bool EnsureDirectory(const std::string &path) {
  if (path.empty()) {
    return false;
  }

  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    return S_ISDIR(st.st_mode);
  }

  if (mkdir(path.c_str(), 0755) == 0) {
    return true;
  }

  return errno == EEXIST;
}

// 构建日志路径
std::string BuildLogPath(const std::string &logDir,
                         const std::string &programName,
                         const std::string &instanceTag, const tm &timeInfo) {
  std::ostringstream oss;
  oss << logDir << "/" << (timeInfo.tm_year + 1900) << "-";
  oss.width(2);
  oss.fill('0');
  oss << (timeInfo.tm_mon + 1) << "-";
  oss.width(2);
  oss.fill('0');
  oss << timeInfo.tm_mday;

  if (!programName.empty()) {
    oss << "-" << programName;
  }
  if (!instanceTag.empty()) {
    oss << "-" << instanceTag;
  }
  oss << ".log";
  return oss.str();
}
} // namespace

Logger::Logger()
    : _logDir("."), _programName("log"), _instanceTag(""), _initialized(false),
      _configEpoch(1), _lockQueue(LoadQueueCapacity()), _shutdown(false) {
  StartConsumer();
}

Logger::~Logger() { Shutdown(); }

bool Logger::LocalTime(time_t now, tm &out) {
#if defined(_WIN32)
  return localtime_s(&out, &now) == 0;
#else
  return localtime_r(&now, &out) != nullptr;
#endif
}

void Logger::StartConsumer() {
  _consumer = std::thread([this]() {
    FILE *pf = nullptr;
    std::string currentPath;

    std::string logDir;
    std::string programName;
    std::string instanceTag;
    unsigned long localEpoch = 0;

    auto refreshConfig = [&]() {
      const unsigned long epoch = _configEpoch.load(std::memory_order_acquire);
      if (epoch != localEpoch) {
        std::lock_guard<std::mutex> lock(_configMutex);
        logDir = _initialized ? _logDir : ".";
        programName = _initialized ? _programName : "log";
        instanceTag = _initialized ? _instanceTag : "";
        localEpoch = epoch;
      }
    };

    auto writeLine = [&](const std::string &line) {
      time_t now = time(nullptr);
      tm nowtm = {};
      if (!Logger::LocalTime(now, nowtm)) {
        std::cerr << line;
        return;
      }

      const std::string filePath =
          BuildLogPath(logDir, programName, instanceTag, nowtm);

      if (pf == nullptr || filePath != currentPath) {
        if (pf != nullptr) {
          fclose(pf);
          pf = nullptr;
        }

        pf = fopen(filePath.c_str(), "a+");
        if (pf == nullptr) {
          std::cerr << "open log file error: " << filePath << std::endl;
          return;
        }
        currentPath = filePath;
      }

      fputs(line.c_str(), pf);
      fflush(pf);
    };

    std::size_t reportedDropped = 0;
    std::string msg;
    while (_lockQueue.Pop(msg)) {
      refreshConfig();

      const std::size_t dropped = _lockQueue.DroppedCount();
      if (dropped > reportedDropped) {
        char note[160] = {0};
        std::snprintf(note, sizeof(note),
                      "[WARN] dropped %zu log message(s) due to full queue\n",
                      dropped - reportedDropped);
        writeLine(note);
        reportedDropped = dropped;
      }
      writeLine(msg);
    }

    if (pf != nullptr) {
      fflush(pf);
      fclose(pf);
      pf = nullptr;
    }
  });
}

Logger &Logger::GetInstance() {
  // 故意泄漏单例：进程退出时不跑析构，避免消费线程在退出阶段访问已销毁的
  // 静态对象。真正需要排空日志时，由普通控制流显式调用 Shutdown()。
  static Logger *logger = new Logger();
  return *logger;
}

bool Logger::Init(const std::string &logDir, const std::string &programName,
                  const std::string &instanceTag) {
  if (IsShutdown()) {
    return false;
  }
  if (!EnsureDirectory(logDir)) {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(_configMutex);
    _logDir = logDir;
    _programName = programName.empty() ? "log" : programName;
    _instanceTag = instanceTag;
    _initialized = true;
  }
  _lockQueue.SetCapacity(LoadQueueCapacity());
  _configEpoch.fetch_add(1, std::memory_order_release);
  return true;
}

void Logger::Shutdown() {
  std::lock_guard<std::mutex> lock(_lifecycleMutex);
  if (_shutdown) {
    return;
  }
  _shutdown = true;
  _lockQueue.Stop();
  if (_consumer.joinable()) {
    _consumer.join();
  }
}

bool Logger::IsShutdown() const {
  std::lock_guard<std::mutex> lock(_lifecycleMutex);
  return _shutdown;
}

std::size_t Logger::DroppedCount() const { return _lockQueue.DroppedCount(); }

std::string Logger::BuildPrefix(LogLevel level) const {
  time_t now = time(nullptr);
  tm nowtm = {};

  char buf[128] = {0};
  if (LocalTime(now, nowtm)) {
    std::snprintf(buf, sizeof(buf), "[%s] %02d:%02d:%02d => ",
                  (level == LogLevel::INFO ? "INFO" : "ERROR"), nowtm.tm_hour,
                  nowtm.tm_min, nowtm.tm_sec);
  } else {
    std::snprintf(buf, sizeof(buf), "[%s] => ",
                  (level == LogLevel::INFO ? "INFO" : "ERROR"));
  }
  return buf;
}

void Logger::Log(const std::string &msg, LogLevel level) {
  if (IsShutdown()) {
    return;
  }

  std::string logMsg = BuildPrefix(level);
  logMsg += msg;
  logMsg += '\n';

  _lockQueue.Push(logMsg);
}
