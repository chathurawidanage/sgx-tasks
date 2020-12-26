cmake_minimum_required(VERSION 3.5)
project(tasker-driver)

get_filename_component(PARENT_DIR ../ ABSOLUTE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${ZeroMQ_INCLUDE_DIR} ${PARENT_DIR}/common/include)

add_library(driver-lib SHARED driver.cpp executor.cpp job.cpp)
target_link_libraries(driver-lib ${ZeroMQ_LIBRARY} tasker-common)