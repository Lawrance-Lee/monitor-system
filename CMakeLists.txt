cmake_minimum_required(VERSION 3.10.2)

project(monitor_system LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 17)

option(BUILD_MANAGER "Build manager server (requires MySQL)" ON)

# Proto 定义
add_subdirectory(proto)

# Worker 后续开发到时再接入
add_subdirectory(worker)

# Manager 后续开发到时再接入
# if(BUILD_MANAGER)
#     add_subdirectory(manager)
# endif()