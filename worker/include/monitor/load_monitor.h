#pragma once
struct LoadInfo{
    double load_1min;
    double load_5min;
    double load_15min;
};
LoadInfo GetLoadInfo();