project(sgx-tasks-driver)

set(SOURCE_FILES main.cpp)
add_executable(
    driver
    main.cpp
)
target_link_libraries(driver PUBLIC ${ZeroMQ_LIBRARY})