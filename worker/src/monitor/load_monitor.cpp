#include "monitor/load_monitor.h"
#include "utils/read_file.h"
#include<sstream>
#include<stdexcept>
LoadInfo GetLoadInfo(){
    std::string content=ReadFile("/proc/loadavg");
    std::stringstream ss(content);
    LoadInfo info{};
    if(!(ss>>info.load_1min
      >>info.load_5min
      >>info.load_15min)){
        throw std::runtime_error("Failed to parse /proc/loadavg");
      }
      return info;
}

