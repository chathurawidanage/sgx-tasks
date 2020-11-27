cmake_minimum_required(VERSION 3.5)
project(tasker-worker)

get_filename_component(PARENT_DIR ../ ABSOLUTE)
message(${PARENT_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${ZeroMQ_INCLUDE_DIR} ${PARENT_DIR}/common/include)

add_library(worker-lib SHARED worker.cpp)
target_link_libraries(worker-lib ${ZeroMQ_LIBRARY} tasker-common)