#include "monitor/host_info_monitor.h"
#include<unistd.h>
#include<arpa/inet.h>
#include<ifaddrs.h>
#include<netinet/in.h>
#include<cstring>

namespace monitor{
    std::string HostInfoMonitor::GetHostname(){
        char hostname[256];
        if(gethostname(hostname,sizeof(hostname))==0){
            return std::string(hostname);
        }
        return "unknown";
    }
    std::string HostInfoMonitor::GetPrimaryIpAddress(){
        struct ifaddrs* ifaddr=nullptr;
        struct ifaddrs* ifa=nullptr;
        std::string result;
        //获取当前系统所有网络信息接口
        if(getifaddrs(&ifaddr)==-1){
            return "";
        }
        for(ifa=ifaddr;ifa!=nullptr;ifa=ifa->ifa_next){
            if(ifa->ifa_addr==nullptr){
                continue;
            }
            //当前只需要ipv4地址
            if(ifa->ifa_addr->sa_family!=AF_INET){
                continue;
            }
            //跳过本地回环接口
            if(std::strcmp(ifa->ifa_name,"lo")==0){
                continue;
            }
            std::string ifname(ifa->ifa_name);
            //跳过常见虚拟网络接口
             if (ifname.find("docker") == 0 ||
        ifname.find("veth") == 0 ||
        ifname.find("br-") == 0 ||
        ifname.find("virbr") == 0) {
      continue;
    }
       auto* addr=
           reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
       char ip_str[INET_ADDRSTRLEN];
       if(inet_ntop(AF_INET,
                     &(addr->sin_addr),
                    ip_str,
                    sizeof(ip_str))!=nullptr){
                        result=ip_str;
                        break;
                    }
        }
    freeifaddrs(ifaddr);
    return result;
}
   void HostInfoMonitor::UpdateOnce(monitor::proto::MonitorInfo* monitor_info){
    if(monitor_info==nullptr){
        return;
    }
    //主机标识信息通常不会频繁变化,只在第一次采集时获取
    if(!info_cached_){
        cached_hostname_=GetHostname();
        cached_ip_=GetPrimaryIpAddress();
        info_cached_=true;
    }
    auto* host_info=monitor_info->mutable_host_info();
    host_info->set_hostname(cached_hostname_);
    host_info->set_ip_address(cached_ip_);
   }
}