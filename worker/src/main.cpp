#include <iostream>
#include "monitor/load_monitor.h"
int main() {
    try{
    LoadInfo info =GetLoadInfo();
    std::cout<<"1 min load: "<<info.load_1min<<std::endl;
    std::cout<<"5 min load: "<<info.load_5min<<std::endl;
    std::cout<<"15 min load: "<<info.load_15min<<std::endl;
}
    catch(const std::exception&e){
        std::cerr<<e.what()<<std::endl;
    }

    return 0;
}