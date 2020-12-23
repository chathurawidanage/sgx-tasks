cmake_minimum_required(VERSION 3.5)
project(sgx-genomic)

include_directories(${SPDLG_INCLUDE_DIR} ${ZMQ_INCLUDE_DIR})
link_directories(${ZMQ_LIBRARY_DIR})

# Server executable
add_executable(
    server
    server.cpp
)
add_dependencies(server spdlog libzmq cppzmq)
target_link_libraries(server PUBLIC zmq ${CMAKE_THREAD_LIBS_INIT} driver-lib tasker-common)

# Worker executable
add_executable(
    insecworker
    insecworker.cpp
)
add_dependencies(insecworker spdlog libzmq cppzmq)
target_link_libraries(insecworker PUBLIC zmq ${CMAKE_THREAD_LIBS_INIT} worker-lib tasker-common)

# Client executable
add_executable(
    client
    client.cpp
)
add_dependencies(client spdlog libzmq cppzmq)
target_link_libraries(client PUBLIC zmq ${CMAKE_THREAD_LIBS_INIT} tasker-common)

# Adding Dida sgx
add_subdirectory(dida-sgx)