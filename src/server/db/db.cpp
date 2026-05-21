#include "../../../include/server/db/db.h"
#include "../../../include/server/logger.h"

#include <cstdlib>

namespace {
// 获取环境变量值，如果环境变量不存在或为空，则返回默认值
std::string getEnvOrDefault(const char *name, const char *defaultValue) {
  // getenv函数用于获取环境变量的值，参数是环境变量的名称，返回值是一个指向环境变量值的指针，如果环境变量不存在，则返回nullptr
  const char *value = std::getenv(name);
  // 如果环境变量不存在或为空，则返回默认值
  if (value == nullptr || value[0] == '\0') {
    return defaultValue;
  }
  return value;
}

unsigned int getEnvPortOrDefault(const char *name, unsigned int defaultValue) {
  // 获取环境变量值
  std::string value = getEnvOrDefault(name, "");
  if (value.empty()) {
    return defaultValue;
  }

  // 尝试将环境变量值转换为整数，并检查是否在有效的端口范围内（1-65535）
  try {
    int port = std::stoi(value);
    if (port > 0 && port <= 65535) {
      return static_cast<unsigned int>(port);
    }
  } catch (...) {
  }

  LOG_ERROR("invalid %s value: %s, fallback to %u", name, value.c_str(),
            defaultValue);
  return defaultValue;
}

} // namespace

// 初始化数据库连接
MySQL::MySQL() { _conn = mysql_init(nullptr); }
// 释放数据库连接
MySQL::~MySQL() { mysql_close(_conn); }
// 连接数据库
bool MySQL::connect() {
  if (_conn == nullptr) {
    LOG_ERROR("mysql_init failed");
    return false;
  }

  const std::string host = getEnvOrDefault("MYSQL_HOST", "127.0.0.1");
  const std::string user = getEnvOrDefault("MYSQL_USER", "chatserver_app");
  const std::string password =
      getEnvOrDefault("MYSQL_PASSWORD", "chatserver_dev_password");
  const std::string database =
      getEnvOrDefault("MYSQL_DATABASE", "chatserver");
  const unsigned int port = getEnvPortOrDefault("MYSQL_PORT", 3306);

  if (mysql_real_connect(_conn, host.c_str(), user.c_str(), password.c_str(),
                         database.c_str(), port, nullptr, 0) != nullptr) {
    // 尝试将数据库连接的字符集设置为 utf8mb4，并检查是否设置成功
    if (mysql_set_character_set(_conn, "utf8mb4") != 0) {
      LOG_ERROR("set mysql charset utf8mb4 failed: %s", mysql_error(_conn));
      return false;
    }

    LOG_INFO("connect mysql success! host=%s port=%u db=%s user=%s",
             host.c_str(), port, database.c_str(), user.c_str());
    return true;
  } else {
    LOG_ERROR("connect mysql fail! host=%s port=%u db=%s user=%s error=%s",
              host.c_str(), port, database.c_str(), user.c_str(),
              mysql_error(_conn));
    return false;
  }
}

// 更新操作
bool MySQL::update(std::string sql) {
  if (mysql_query(_conn, sql.c_str())) {
    // __FILE__代表当前文件名(字符串)，__LINE__代表当前行号(整数)
    LOG_ERROR("%s:%d:%s更新失败！ mysql error: %s", __FILE__, __LINE__,
              sql.c_str(), mysql_error(_conn));
    return false;
  }
  return true;
}

// 查询操作
MYSQL_RES *MySQL::query(std::string sql) {
  if (mysql_query(_conn, sql.c_str())) {
    LOG_ERROR("%s:%d:%s查询失败！ mysql error: %s", __FILE__, __LINE__,
              sql.c_str(), mysql_error(_conn));
    return nullptr;
  }
  return mysql_store_result(_conn);
}

// 获取连接
MYSQL *MySQL::getconnection() { return _conn; }
