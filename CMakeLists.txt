cmake_minimum_required(VERSION 3.5)
project(sgx-tasks)

set(CMAKE_CXX_STANDARD 17)

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

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

add_subdirectory(common)
include_directories(common/include)

# Driver executable
add_subdirectory(driver)
include_directories(driver/include)

add_executable(
    server
    server.cpp
)
target_link_libraries(server PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} driver-lib tasker-common)

# Worker executable
add_subdirectory(worker)
include_directories(worker/include)
add_executable(
    task
    task.cpp
)
target_link_libraries(task PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} worker-lib tasker-common)

# Client executable
add_executable(
    client
    client.cpp
)
target_link_libraries(client PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} tasker-common)

