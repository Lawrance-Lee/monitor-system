#pragma once

#include "monitor_info.pb.h"

namespace monitor{

    class MonitorInter{
        public:
        MonitorInter(){}
        
        virtual ~MonitorInter(){}

        //所有监控器都通过该接口完成一次指标采集,并写入统一的MonitorInfo
        virtual void UpdateOnce(monitor::proto::MonitorInfo* monitor_info)=0;

        //统一停止监控器并释放其持有的运行资源
        virtual void Stop()=0;
  };
  }//namespace monitor