cmake_minimum_required(VERSION 3.5)
project(sgx-tasks)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
pkg_check_modules(PC_ZeroMQ QUIET zmq)

find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.hpp
        PATHS ${PC_ZeroMQ_INCLUDE_DIRS})

find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS})

include_directories(SYSTEM ${ZeroMQ_INCLUDE_DIR})

find_package(Threads)

# Driver executable
add_executable(
    driver
    driver.cpp
)
target_link_libraries(driver PUBLIC ${ZeroMQ_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

# Worker executable
add_executable(
    worker
    worker.cpp
)
target_link_libraries(worker PUBLIC ${ZeroMQ_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

# Client executable
add_executable(
    client
    client.cpp
)
target_link_libraries(client PUBLIC ${ZeroMQ_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

