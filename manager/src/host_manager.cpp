#include "host_manager.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

#ifdef ENABLE_MYSQL
#include <mysql/mysql.h>
#endif

namespace monitor {

// 用于网络速率计算的采样数据
struct NetSample {
  double last_in_bytes = 0;
  double last_out_bytes = 0;
  std::chrono::system_clock::time_point last_time;
};

static std::map<std::string, NetSample> net_samples;

// 用于变化率计算的性能采样数据
struct PerfSample {
  float cpu_percent = 0, usr_percent = 0, system_percent = 0;
  float nice_percent = 0, idle_percent = 0, io_wait_percent = 0;
  float irq_percent = 0, soft_irq_percent = 0;
  float steal_percent = 0, guest_percent = 0, guest_nice_percent = 0;
  float load_avg_1 = 0, load_avg_3 = 0, load_avg_15 = 0;
  float mem_used_percent = 0, mem_total = 0, mem_free = 0, mem_avail = 0;
  float net_in_rate = 0, net_out_rate = 0;
  float score = 0;
};

static std::map<std::string, PerfSample> last_perf_samples;

HostManager::HostManager() : running_(false) {
}

HostManager::~HostManager() {
  Stop();
}

void HostManager::Start() {
  running_ = true;
  thread_ = std::make_unique<std::thread>(&HostManager::ProcessLoop, this);
}

void HostManager::Stop() {
  running_ = false;
  if (thread_ && thread_->joinable()) {
    thread_->join();
  }
}

void HostManager::ProcessLoop() {
  while (running_) {
    std::this_thread::sleep_for(std::chrono::seconds(60));

    auto now = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = host_scores_.begin(); it != host_scores_.end();) {
      auto age = std::chrono::duration_cast<std::chrono::seconds>(
          now - it->second.timestamp).count();

      if (age > 60) {
        std::cout << "Removing stale host: " << it->first << std::endl;
        it = host_scores_.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void HostManager::OnDataReceived(
    const monitor::proto::MonitorInfo& info) {
  // 构建服务器唯一标识: hostname_ip
  std::string host_name;

  if (info.has_host_info()) {
    const auto& host_info = info.host_info();
    std::string hostname = host_info.hostname();
    std::string ip = host_info.ip_address();

    if (!hostname.empty() && !ip.empty()) {
      host_name = hostname + "_" + ip;
    } else if (!hostname.empty()) {
      host_name = hostname;
    } else if (!ip.empty()) {
      host_name = ip;
    }
  }

  // 兼容旧版本：如果 host_info 为空，使用 name 字段
  if (host_name.empty()) {
    host_name = info.name();
  }

  if (host_name.empty()) {
    std::cerr << "Received data with empty server identifier"
              << std::endl;
    return;
  }

  double score = CalcScore(info);
  auto now = std::chrono::system_clock::now();

  // 网络速率计算
  double net_in_rate = 0, net_out_rate = 0;

  if (info.net_info_size() > 0) {
    net_in_rate =
        info.net_info(0).rcv_rate() / (1024.0 * 1024.0);

    net_out_rate =
        info.net_info(0).send_rate() / (1024.0 * 1024.0);
  }

  // 当前采样
  PerfSample curr;

  if (info.cpu_stat_size() > 0) {
    const auto& cpu = info.cpu_stat(0);

    curr.cpu_percent = cpu.cpu_percent();
    curr.usr_percent = cpu.usr_percent();
    curr.system_percent = cpu.system_percent();
    curr.nice_percent = cpu.nice_percent();
    curr.idle_percent = cpu.idle_percent();
    curr.io_wait_percent = cpu.io_wait_percent();
    curr.irq_percent = cpu.irq_percent();
    curr.soft_irq_percent = cpu.soft_irq_percent();
  }

  if (info.has_cpu_load()) {
    curr.load_avg_1 = info.cpu_load().load_avg_1();
    curr.load_avg_3 = info.cpu_load().load_avg_3();
    curr.load_avg_15 = info.cpu_load().load_avg_15();
  }

  if (info.has_mem_info()) {
    curr.mem_used_percent = info.mem_info().used_percent();
    curr.mem_total = info.mem_info().total();
    curr.mem_free = info.mem_info().free();
    curr.mem_avail = info.mem_info().avail();
  }

  curr.net_in_rate = net_in_rate;
  curr.net_out_rate = net_out_rate;
  curr.score = score;

  // 变化率计算
  PerfSample last = last_perf_samples[host_name];

  auto rate = [](float now_val, float last_val) -> float {
    if (last_val == 0) {
      return 0;
    }

    return (now_val - last_val) / last_val;
  };

  float cpu_percent_rate =
      rate(curr.cpu_percent, last.cpu_percent);

  float usr_percent_rate =
      rate(curr.usr_percent, last.usr_percent);

  float system_percent_rate =
      rate(curr.system_percent, last.system_percent);

  float nice_percent_rate =
      rate(curr.nice_percent, last.nice_percent);

  float idle_percent_rate =
      rate(curr.idle_percent, last.idle_percent);

  float io_wait_percent_rate =
      rate(curr.io_wait_percent, last.io_wait_percent);

  float irq_percent_rate =
      rate(curr.irq_percent, last.irq_percent);

  float soft_irq_percent_rate =
      rate(curr.soft_irq_percent, last.soft_irq_percent);

  float load_avg_1_rate =
      rate(curr.load_avg_1, last.load_avg_1);

  float load_avg_3_rate =
      rate(curr.load_avg_3, last.load_avg_3);

  float load_avg_15_rate =
      rate(curr.load_avg_15, last.load_avg_15);

  float mem_used_percent_rate =
      rate(curr.mem_used_percent, last.mem_used_percent);

  float mem_total_rate =
      rate(curr.mem_total, last.mem_total);

  float mem_free_rate =
      rate(curr.mem_free, last.mem_free);

  float mem_avail_rate =
      rate(curr.mem_avail, last.mem_avail);

  float net_in_rate_rate =
      rate(curr.net_in_rate, last.net_in_rate);

  float net_out_rate_rate =
      rate(curr.net_out_rate, last.net_out_rate);

  last_perf_samples[host_name] = curr;

  {
    std::lock_guard<std::mutex> lock(mtx_);
    host_scores_[host_name] = HostScore{info, score, now};
  }

  // 写入所有表
  WriteToMysql(
      host_name,
      HostScore{info, score, now},
      net_in_rate,
      net_out_rate,
      cpu_percent_rate,
      usr_percent_rate,
      system_percent_rate,
      nice_percent_rate,
      idle_percent_rate,
      io_wait_percent_rate,
      irq_percent_rate,
      soft_irq_percent_rate,
      0,
      0,
      0,
      load_avg_1_rate,
      load_avg_3_rate,
      load_avg_15_rate,
      mem_used_percent_rate,
      mem_total_rate,
      mem_free_rate,
      mem_avail_rate,
      net_in_rate_rate,
      net_out_rate_rate,
      0,
      0);

  std::cout << "\n================== Received Data =================="
            << std::endl;

  std::cout << "Server: " << host_name
            << ", Score: " << score << std::endl;

  // CPU 详细信息
  std::cout << "\n--- CPU ---" << std::endl;

  std::cout << "  Usage: " << curr.cpu_percent << "%, "
            << "User: " << curr.usr_percent << "%, "
            << "System: " << curr.system_percent << "%"
            << std::endl;

  std::cout << "  Nice: " << curr.nice_percent << "%, "
            << "Idle: " << curr.idle_percent << "%, "
            << "IOWait: " << curr.io_wait_percent << "%"
            << std::endl;

  std::cout << "  IRQ: " << curr.irq_percent << "%, "
            << "SoftIRQ: " << curr.soft_irq_percent << "%"
            << std::endl;

  std::cout << "  Load: "
            << curr.load_avg_1 << "/"
            << curr.load_avg_3 << "/"
            << curr.load_avg_15
            << std::endl;

  // 内存详细信息
  std::cout << "\n--- Memory ---" << std::endl;

  std::cout << "  Used: " << curr.mem_used_percent << "%, "
            << "Total: " << curr.mem_total << " MB"
            << std::endl;

  std::cout << "  Free: " << curr.mem_free << " MB, "
            << "Avail: " << curr.mem_avail << " MB"
            << std::endl;

  // 网络详细信息
  std::cout << "\n--- Network ---" << std::endl;

  std::cout << "  In: " << net_in_rate * 1024 * 1024 << " B/s, "
            << "Out: " << net_out_rate * 1024 * 1024 << " B/s"
            << std::endl;

  for (int i = 0; i < info.net_info_size(); ++i) {
    const auto& net = info.net_info(i);

    std::cout << "  [" << net.name() << "] Recv: "
              << net.rcv_rate() << " B/s, "
              << "Send: " << net.send_rate() << " B/s, "
              << "Drops: " << net.drop_in() << "/"
              << net.drop_out() << std::endl;
  }

  // 磁盘详细信息
  std::cout << "\n--- Disk ---" << std::endl;

  float max_disk_util = 0;

  for (int i = 0; i < info.disk_info_size(); ++i) {
    const auto& disk = info.disk_info(i);

    std::cout << "  [" << disk.name() << "] "
              << "Read: "
              << disk.read_bytes_per_sec() / 1024.0 << " KB/s, "
              << "Write: "
              << disk.write_bytes_per_sec() / 1024.0 << " KB/s, "
              << "Util: "
              << disk.util_percent() << "%"
              << std::endl;

    if (disk.util_percent() > max_disk_util) {
      max_disk_util = disk.util_percent();
    }
  }

  if (info.disk_info_size() == 0) {
    std::cout << "  No disk data" << std::endl;
  }

  // 软中断信息
  std::cout << "\n--- SoftIRQ ---" << std::endl;

  std::cout << "  CPU cores with softirq data: "
            << info.soft_irq_size() << std::endl;

  // 变化率信息
  std::cout << "\n--- Change Rates ---" << std::endl;

  std::cout << "  CPU: " << cpu_percent_rate * 100 << "%, "
            << "Mem: " << mem_used_percent_rate * 100 << "%, "
            << "Load: " << load_avg_1_rate * 100 << "%"
            << std::endl;

  std::cout << "  NetIn: " << net_in_rate_rate * 100 << "%, "
            << "NetOut: " << net_out_rate_rate * 100 << "%"
            << std::endl;

  std::cout << "\n--- Database ---" << std::endl;

  std::cout << "  Data saved to MySQL (monitor_db)"
            << std::endl;

  std::cout << "====================================================\n"
            << std::endl;
}

std::unordered_map<std::string, HostScore>
HostManager::GetAllHostScores() {
  std::lock_guard<std::mutex> lock(mtx_);
  return host_scores_;
}

std::string HostManager::GetBestHost() {
  std::lock_guard<std::mutex> lock(mtx_);

  std::string best_host;
  double best_score = -1;

  for (const auto& [host, data] : host_scores_) {
    if (data.score > best_score) {
      best_score = data.score;
      best_host = host;
    }
  }

  return best_host;
}
double HostManager::CalcScore(
    const monitor::proto::MonitorInfo& info) {
  // ============================================================
  // 性能评分模型 - 针对学校选课/查成绩系统高并发场景优化
  // ============================================================
  // 权重配置：
  // - CPU 使用率: 35%
  // - 内存使用率: 30%
  // - CPU 负载:   15%
  // - 磁盘 IO:    15%
  // - 网络带宽:    5% (收发各 2.5%)
  // ============================================================

  const double cpu_weight = 0.35;
  const double mem_weight = 0.30;
  const double load_weight = 0.15;
  const double disk_weight = 0.15;
  const double net_weight = 0.05;

  const double load_coefficient = 1.5;
  const double max_bandwidth = 125000000.0;

  double cpu_percent = 0;
  double load_avg_1 = 0;
  double mem_percent = 0;

  double net_recv_rate = 0;
  double net_send_rate = 0;
  double disk_util = 0;

  int cpu_cores = 1;
    if (info.cpu_stat_size() > 0) {
    cpu_percent = info.cpu_stat(0).cpu_percent();

    cpu_cores = info.cpu_stat_size() - 1;

    if (cpu_cores < 1) {
      cpu_cores = 1;
    }
  }

  if (info.has_cpu_load()) {
    load_avg_1 = info.cpu_load().load_avg_1();
  }

  if (info.has_mem_info()) {
    mem_percent = info.mem_info().used_percent();
  }

  if (info.net_info_size() > 0) {
    net_recv_rate = info.net_info(0).rcv_rate();
    net_send_rate = info.net_info(0).send_rate();
  }

  if (info.disk_info_size() > 0) {
    for (int i = 0; i < info.disk_info_size(); ++i) {
      double util = info.disk_info(i).util_percent();

      if (util > disk_util) {
        disk_util = util;
      }
    }
  }
    // 反向归一化
  auto clamp = [](double v) {
    return v < 0 ? 0 : (v > 1 ? 1 : v);
  };

  double cpu_score =
      clamp(1.0 - cpu_percent / 100.0);

  double mem_score =
      clamp(1.0 - mem_percent / 100.0);

  double load_score =
      clamp(1.0 -
            load_avg_1 /
                (cpu_cores * load_coefficient));

  double disk_score =
      clamp(1.0 - disk_util / 100.0);

  double net_recv_score =
      clamp(1.0 - net_recv_rate / max_bandwidth);

  double net_send_score =
      clamp(1.0 - net_send_rate / max_bandwidth);

  double net_score =
      (net_recv_score + net_send_score) / 2.0;
        double score =
      cpu_score * cpu_weight +
      mem_score * mem_weight +
      load_score * load_weight +
      disk_score * disk_weight +
      net_score * net_weight;

  score *= 100.0;

  return score < 0 ? 0 :
         (score > 100 ? 100 : score);
}

}  // namespace monitor