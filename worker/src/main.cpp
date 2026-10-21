#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>

#include "rpc/monitor_pusher.h"

constexpr char kDefaultManagerAddress[] = "localhost:50051";
constexpr int kDefaultPushInterval = 10;  // 秒

volatile std::sig_atomic_t g_running = 1;

void HandleSignal(int signal) {
  g_running = 0;
}

void PrintUsage(const char* program) {
  std::cout << "Usage: " << program << " <manager_address> [interval_seconds]"
            << std::endl;
  std::cout << "  manager_address: 管理者服务器地址 (如 192.168.1.100:50051)"
            << std::endl;
  std::cout << "  interval_seconds: 推送间隔秒数 (默认 10)" << std::endl;
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  std::string manager_address = kDefaultManagerAddress;
  int interval_seconds = kDefaultPushInterval;

  // 解析命令行参数
  if (argc > 1) {
    manager_address = argv[1];
  }
  if (argc > 2) {
    interval_seconds = std::stoi(argv[2]);
    if (interval_seconds <= 0) {
      interval_seconds = kDefaultPushInterval;
    }
  }

  std::cout << "Starting Monitor Server (Push Mode)..." << std::endl;
  std::cout << "Manager address: " << manager_address << std::endl;
  std::cout << "Push interval: " << interval_seconds << " seconds" << std::endl;

  // 创建并启动推送器
  monitor::MonitorPusher pusher(manager_address, interval_seconds);
  pusher.Start();

  // 主线程保持运行，收到 SIGINT/SIGTERM 后正常退出
  std::cout << "Press Ctrl+C to exit." << std::endl;
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  pusher.Stop();

  return 0;
}