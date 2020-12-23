cmake_minimum_required(VERSION 3.5)
project(tasker-driver)

get_filename_component(PARENT_DIR ../ ABSOLUTE)
message(${PARENT_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${ZMQ_INCLUDE_DIR} ${PARENT_DIR}/common/include ${SPDLG_INCLUDE_DIR})
link_directories($ZMQ_LIBRARY_DIR)

add_library(driver-lib STATIC driver.cpp executor.cpp job.cpp)
add_dependencies(driver-lib spdlog libzmq cppzmq)
target_link_libraries(driver-lib zmq tasker-common)