cmake_minimum_required(VERSION 3.27)
project(4)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(4
        client/main.cpp
        server/main.cpp
        common.h)
