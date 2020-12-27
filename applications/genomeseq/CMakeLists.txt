cmake_minimum_required(VERSION 3.5)
project(sgx-genomic)

add_library(commands SHARED commands.cpp)

# Server executable
add_executable(
    server
    server.cpp
)
target_link_libraries(server PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} driver-lib tasker-common commands)

# Worker executable
add_executable(
    unsecworker
    unsecworker.cpp
)
target_link_libraries(unsecworker PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} worker-lib tasker-common commands)

# Client executable
add_executable(
    client
    client.cpp
)
target_link_libraries(client PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} tasker-common)

# to make commands.hpp available for dida-sgx
include_directories(".")

# Adding Dida sgx
add_subdirectory(dida-sgx)