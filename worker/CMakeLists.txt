project(sgx-tasks-worker)

set(SOURCE_FILES main.cpp)
add_executable(
    worker
    main.cpp
)
target_link_libraries(worker PUBLIC ${ZeroMQ_LIBRARY})