# Linux C++ Performance Monitoring System

基于 C++17 实现的 Linux 服务器性能监控系统，采用 Worker / Manager / Query Client 分离架构，实现多维性能指标采集、远程数据上报、MySQL 持久化以及历史性能查询。

本项目基于参考源码进行学习复刻，并完成了模块实现、环境适配、编译调试和端到端联调。

## 1. 技术栈

- **开发语言**：C++17、C
- **操作系统**：Linux（Ubuntu 24.04）
- **构建工具**：CMake、GCC、Make
- **远程通信**：gRPC、Protocol Buffers
- **性能采集**：Linux `/proc`、内核模块、字符设备、mmap
- **网络监控**：eBPF、TC、BPF Map、libbpf
- **数据库**：MySQL 8.0
- **并发管理**：std::thread、mutex、atomic、RAII

## 2. 系统架构

系统包含三个主要程序：

- **Worker**：部署在被监控 Linux 主机上，周期性采集 CPU、内存、磁盘、网络和软中断等指标。
- **Manager**：接收 Worker 上报的数据，进行主机管理、性能评分和数据库持久化，同时提供查询服务。
- **Query Client**：通过 gRPC 请求 Manager，查询指定主机的历史性能记录。

### 数据流

```text
Linux /proc、内核模块、eBPF
              |
              v
        Worker Monitors
              |
              v
       MetricCollector
              |
              v
        MonitorPusher
              |
       Protobuf + gRPC
              |
              v
        Manager Server
              |
              v
         HostManager
              |
              v
          MySQL
              ^
              |
         QueryManager
              ^
              |
       gRPC QueryService
              ^
              |
         Query Client
```

## 3. 核心功能

### Linux 性能指标采集

- CPU 使用率、用户态/内核态占比、CPU Load
- 内存总量、空闲内存、可用内存及使用率
- 磁盘读写速率、IOPS、读写延迟、利用率
- 网络收发字节数、数据包数量与传输速率
- 各 CPU 核心的软中断统计

### 内核态性能监控

通过 Linux 内核模块采集 CPU Stat 和 SoftIRQ 数据，利用字符设备与 mmap 向用户态提供统计结果。

### eBPF 网络监控

利用 TC ingress/egress 挂载 eBPF 程序，在内核网络路径统计收发字节数和数据包数量。

通过 BPF Map 保存统计数据，由用户态采集器读取并计算网络速率。

### gRPC 数据通信

通过 Protobuf 定义监控消息和远程服务接口。

Worker 定期向 Manager 推送监控数据，Manager 提供独立的性能查询 RPC 服务。

### 数据持久化与查询

MySQL 使用五张表保存监控数据：

| 数据表 | 功能 |
|---|---|
| `server_performance` | 综合性能指标与评分 |
| `server_mem_detail` | 内存详细指标 |
| `server_disk_detail` | 磁盘性能明细 |
| `server_net_detail` | 网络性能明细 |
| `server_softirq_detail` | 软中断统计明细 |

查询服务代码包含历史性能、趋势、异常、评分及各类详细指标查询接口。

命令行 Query Client 已实现指定主机、时间范围和分页条件的历史性能查询。

## 4. 项目目录

```text
monitor_system/
├── CMakeLists.txt
├── proto/                  # Protobuf 消息与 RPC 定义
├── worker/
│   ├── include/            # Worker 头文件
│   ├── src/
│   │   ├── monitor/        # 性能采集器
│   │   ├── ebpf/           # eBPF 网络监控
│   │   ├── kmod/           # Linux 内核模块
│   │   ├── rpc/            # gRPC 推送模块
│   │   └── main.cpp
│   └── scripts/            # 内核模块管理脚本
├── manager/
│   ├── include/
│   ├── src/
│   │   ├── rpc/            # 接收与查询服务
│   │   ├── host_manager.cpp
│   │   ├── query_manager.cpp
│   │   ├── query_main.cpp
│   │   └── main.cpp
│   └── sql/                # MySQL 建表脚本
└── README.md
```

## 5. 构建与运行

### 环境要求

推荐 Ubuntu Linux，依赖 CMake、C++17 编译器、Protobuf、gRPC 和 MySQL 客户端开发库。

启用 eBPF 时，还需要 Clang、bpftool、libbpf、libelf、相关内核头文件及对应的 `vmlinux.h`。

### 编译项目

```bash
cmake -S . -B build
cmake --build build -j2
```

### 初始化数据库

```bash
sudo mysql < manager/sql/init_server_performance.sql
```

需要预先创建具备 `monitor_db` 相应访问权限的 MySQL 用户。

### 编译并加载内核模块

```bash
sudo bash worker/scripts/load_modules.sh load
```

### 启动 Manager

先在终端安全设置数据库密码环境变量：

```bash
read -rs -p "MySQL password: " MONITOR_DB_PASSWORD
echo
export MONITOR_DB_PASSWORD
```

然后启动：

```bash
./build/manager/manager
```

默认监听 `0.0.0.0:50051`。

### 启动 Worker

在另一个终端运行：

```bash
sudo ./build/worker/worker localhost:50051 5
```

示例中的 `5` 表示每 5 秒采集并推送一次数据。eBPF 加载和 TC 挂载需要相应系统权限。

### 查询历史记录

```bash
./build/manager/query_client \
  localhost:50051 \
  <server_name> \
  2026-11-04T00:00:00+08:00 \
  2026-11-05T00:00:00+08:00 \
  1 5
```

将 `<server_name>` 替换为数据库中实际记录的服务器标识。

## 6. 实际验证

在 Ubuntu 虚拟机环境中完成过以下验证：

- Worker → gRPC → Manager → MySQL 数据链路正常运行
- 五张监控表均成功写入真实采样数据
- CPU Stat、SoftIRQ 内核模块能够加载并采集数据
- eBPF 程序成功挂载至网卡 TC ingress/egress
- Worker 正常退出后，相关 TC 过滤器得到清理
- Query Client 成功查询并展示历史性能记录
- Worker、Manager 可通过 Ctrl+C 正常结束

以上为本地功能验证，不代表完成了长期稳定性、压力测试或生产环境部署。

## 7. 项目说明

这是一个以 Linux C++ 系统编程和性能监控为核心的学习实践项目，主要用于理解用户态与内核态交互、eBPF 网络监控、gRPC 通信、数据库持久化及 C++ 资源管理。

当前 gRPC 服务使用非加密连接，部署示例面向受信任的本地测试环境，不建议直接暴露到公网。

## License

MIT License。详见 [LICENSE](LICENSE)。
