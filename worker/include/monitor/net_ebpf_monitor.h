#pragma once
#include<vector>
#include<string>
#include<unordered_map>
#include<chrono>
#include<memory>
#include<cstdint>
#include "monitor/monitor_inter.h"
struct bpf_object;
namespace monitor{
class NetEbpfMonitor:public MonitorInter{
    public:
    NetEbpfMonitor();
    ~NetEbpfMonitor() override;
    void UpdateOnce(monitor::proto::MonitorInfo* monitor_info)override;
    void Stop()override;
    bool IsLoaded() const{return loaded_;}
    private:
    bool InitEbpf();
    void CleanupEbpf();
    std::string GetIfName(uint32_t ifindex);
    struct NetStatCache{
        uint64_t rcv_bytes;
        uint64_t rcv_packets;
        uint64_t snd_bytes;
        uint64_t snd_packets;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<uint32_t,NetStatCache>cache_;
    std::unordered_map<uint32_t,std::string>ifname_cache_;
   struct TcAttachInfo {
    uint32_t ifindex;

    uint32_t ingress_handle;
    uint32_t ingress_priority;

    uint32_t egress_handle;
    uint32_t egress_priority;
};

std::vector<TcAttachInfo> attached_tc_;
    struct bpf_object* bpf_obj_=nullptr;
    int map_fd_=-1;
    bool loaded_=false;
    std::chrono::steady_clock::time_point last_update_;
};

}
