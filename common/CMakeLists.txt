cmake_minimum_required(VERSION 3.5)
project(tasker-common)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(tasker-common SHARED messages.cpp)