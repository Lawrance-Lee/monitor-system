#include <iostream>
#include"utils/read_file.h"
int main() {
    try{
    std::string content=ReadFile("/proc/loadavg");
    std::cout<<content<<std::endl;}
    catch(const std::exception&e){
        std::cerr<<e.what()<<std::endl;
    }

    return 0;
}