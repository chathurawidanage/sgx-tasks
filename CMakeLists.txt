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

# Logging
include_directories(spdlog/include)
# if(NOT TARGET spdlog)
#     find_package(spdlog REQUIRED)
# endif()

# Common
add_subdirectory(common)
include_directories(common/include)

# Driver
add_subdirectory(driver)
include_directories(driver/include)

# Worker
add_subdirectory(worker)
include_directories(worker/include)

# Applications
add_subdirectory(applications)

