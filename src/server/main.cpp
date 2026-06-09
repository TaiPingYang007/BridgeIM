#include "../../include/server/chatserver.hpp"
#include "../../include/server/chatservice.hpp"
#include "../../include/server/logger.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#ifndef CHATSERVER_LOG_DIR
#define CHATSERVER_LOG_DIR "."
#endif

namespace {

muduo::net::EventLoop *g_loop = nullptr;

void resetHandler(int) {
  if (g_loop != nullptr) {
    g_loop->quit();
  }
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "command invalid! example: ./ChatServer 0.0.0.0 6000"
              << "\n";
    exit(-1);
  }

  // 解析通过命令行参数传递的ip和port
  char *ip = argv[1];
  uint16_t port = atoi(argv[2]);

  // 初始化日志系统
  if (!Logger::GetInstance().Init(CHATSERVER_LOG_DIR, "chatserver",
                                  std::to_string(port))) {
    std::cerr << "logger init failed! log dir: " << CHATSERVER_LOG_DIR << "\n";
    exit(EXIT_FAILURE);
  }

  // 初始化 ChatService
  ChatService::instance();

  // 创建 Muduo EventLoop
  muduo::net::EventLoop loop;
  g_loop = &loop;

  // 注册 Ctrl+C 退出处理函数，确保服务器在接收到 SIGINT 信号时退出 EventLoop，
  // 并回到 main 的普通控制流执行业务重置和日志收尾。
  std::signal(SIGINT, resetHandler);

  // 创建服务器地址对象，绑定指定的ip和port
  muduo::net::InetAddress addr(ip, port);
  // 创建 ChatServer 对象，传入事件循环对象、服务器地址和服务器名称
  ChatServer server(&loop, addr, "ChatServer");

  server.start();
  LOG_INFO("ChatServer started at %s:%d\n", ip, port);
  loop.loop();

  LOG_INFO("ChatServer stopping at %s:%d\n", ip, port);
  ChatService::instance()->reset();
  LOG_INFO("ChatServer stopped at %s:%d\n", ip, port);
  Logger::GetInstance().Shutdown();

  return 0;
}
