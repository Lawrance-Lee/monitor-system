#include "monitor/cpu_stat_monitor.h"
#include "monitor/monitor_structs.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"

namespace monitor {

void CpuStatMonitor::UpdateOnce(monitor::proto::MonitorInfo* monitor_info) {
    int fd = open("/dev/cpu_stat_monitor", O_RDONLY);
    if (fd < 0) {
        return;
    }

    size_t stat_count = 256;
    size_t stat_size = sizeof(struct cpu_stat) * stat_count;

    void* addr = mmap(nullptr, stat_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return;
    }

    struct cpu_stat* stats = static_cast<struct cpu_stat*>(addr);

    for (size_t i = 0; i < stat_count; ++i) {
        if (stats[i].cpu_name[0] == '\0') {
            break;
        }

        auto it = cpu_stat_map_.find(stats[i].cpu_name);

        if (it != cpu_stat_map_.end()) {
            struct CpuStat old = it->second;

            uint64_t new_cpu_total_time =
                stats[i].user +
                stats[i].system +
                stats[i].idle +
                stats[i].nice +
                stats[i].iowait +
                stats[i].irq +
                stats[i].softirq +
                stats[i].steal;

            uint64_t old_cpu_total_time =
                old.user +
                old.system +
                old.idle +
                old.nice +
                old.io_wait +
                old.irq +
                old.soft_irq +
                old.steal;

            uint64_t new_cpu_busy_time =
                stats[i].user +
                stats[i].system +
                stats[i].nice +
                stats[i].irq +
                stats[i].softirq +
                stats[i].steal;

            uint64_t old_cpu_busy_time =
                old.user +
                old.system +
                old.nice +
                old.irq +
                old.soft_irq +
                old.steal;

            if (new_cpu_total_time > old_cpu_total_time) {
                double total_delta =
                    static_cast<double>(new_cpu_total_time - old_cpu_total_time);

                float cpu_percent =
                    static_cast<float>(
                        (new_cpu_busy_time - old_cpu_busy_time) /
                        total_delta * 100.0);

                float cpu_user_percent =
                    static_cast<float>(
                        (stats[i].user - old.user) /
                        total_delta * 100.0);

                float cpu_system_percent =
                    static_cast<float>(
                        (stats[i].system - old.system) /
                        total_delta * 100.0);

                float cpu_nice_percent =
                    static_cast<float>(
                        (stats[i].nice - old.nice) /
                        total_delta * 100.0);

                float cpu_idle_percent =
                    static_cast<float>(
                        (stats[i].idle - old.idle) /
                        total_delta * 100.0);

                float cpu_io_wait_percent =
                    static_cast<float>(
                        (stats[i].iowait - old.io_wait) /
                        total_delta * 100.0);

                float cpu_irq_percent =
                    static_cast<float>(
                        (stats[i].irq - old.irq) /
                        total_delta * 100.0);

                float cpu_soft_irq_percent =
                    static_cast<float>(
                        (stats[i].softirq - old.soft_irq) /
                        total_delta * 100.0);

                auto* cpu_stat_msg = monitor_info->add_cpu_stat();

                cpu_stat_msg->set_cpu_name(stats[i].cpu_name);
                cpu_stat_msg->set_cpu_percent(cpu_percent);
                cpu_stat_msg->set_usr_percent(cpu_user_percent);
                cpu_stat_msg->set_system_percent(cpu_system_percent);
                cpu_stat_msg->set_nice_percent(cpu_nice_percent);
                cpu_stat_msg->set_idle_percent(cpu_idle_percent);
                cpu_stat_msg->set_io_wait_percent(cpu_io_wait_percent);
                cpu_stat_msg->set_irq_percent(cpu_irq_percent);
                cpu_stat_msg->set_soft_irq_percent(cpu_soft_irq_percent);
            }
        }

        CpuStat& cached = cpu_stat_map_[stats[i].cpu_name];
        cached.cpu_name = stats[i].cpu_name;
        cached.user = stats[i].user;
        cached.nice = stats[i].nice;
        cached.system = stats[i].system;
        cached.idle = stats[i].idle;
        cached.io_wait = stats[i].iowait;
        cached.irq = stats[i].irq;
        cached.soft_irq = stats[i].softirq;
        cached.steal = stats[i].steal;
        cached.guest = stats[i].guest;
        cached.guest_nice = stats[i].guest_nice;
    }

    munmap(addr, stat_size);
    close(fd);
}

}  // namespace monitor