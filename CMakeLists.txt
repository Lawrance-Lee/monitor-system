cmake_minimum_required(VERSION 3.16)

project(monitor_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(worker 
    worker/src/main.cpp
    worker/src/utils/read_file.cpp
)

target_include_directories(worker PRIVATE worker/include)
