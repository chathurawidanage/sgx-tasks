cmake_minimum_required(VERSION 3.5)
project(sgx-genomic)

# Server executable
add_executable(
    server
    server.cpp
)
target_link_libraries(server PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} driver-lib tasker-common)

# Worker executable
add_executable(
    insecworker
    insecworker.cpp
)
target_link_libraries(insecworker PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} worker-lib tasker-common)

# Client executable
add_executable(
    client
    client.cpp
)
target_link_libraries(client PUBLIC ${ZeroMQ_LIBRARY} spdlog::spdlog ${CMAKE_THREAD_LIBS_INIT} tasker-common)

# Adding Dida sgx
add_subdirectory(dida-sgx)