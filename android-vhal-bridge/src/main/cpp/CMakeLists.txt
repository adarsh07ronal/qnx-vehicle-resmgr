cmake_minimum_required(VERSION 3.22.1)
project(vhal_bridge)

add_library(vhal_bridge SHARED
    vhal_bridge.cpp)

find_library(log-lib log)

target_link_libraries(vhal_bridge ${log-lib})
