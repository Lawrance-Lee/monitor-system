#pragma once
#include<string>
#include "monitor/monitor_inter.h"

namespace monitor{
    class HostInfoMonitor:public MonitorInter{
     public:
        HostInfoMonitor()=default;
        ~HostInfoMonitor() override=default;
        //采集一次主机标识信息,并写入MonitorInfo
        void UpdateOnce(monitor::proto::MonitorInfo* monitor_info) override;
        void Stop() override {}
    private:
    //获取主机名
        std::string GetHostname();
    //获取主要的非loopbackIPV4地址
    std::string GetPrimaryIpAddress();
    std::string cached_hostname_;
    std::string cached_ip_;
    bool info_cached_=false;
    };
}
