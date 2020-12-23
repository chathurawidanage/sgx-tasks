cmake_minimum_required(VERSION 3.5)
project(sgx-tasks)

set(CMAKE_CXX_STANDARD 17)

include(ExternalProject)

# zeromq
# ExternalProject_Add(libzmq
#     GIT_REPOSITORY https://github.com/zeromq/libzmq.git
#     CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
# )



set(EXTERNAL_INSTALL_LOCATION ${PROJECT_SOURCE_DIR}/external)
set(SPDLG_ROOT ${EXTERNAL_INSTALL_LOCATION}/spdlog)
set(SPDLG_INCLUDE_DIR ${SPDLG_ROOT}/include)
set(SPDLG_LIBRARY ${SPDLG_ROOT}/lib)

ExternalProject_Add(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.8.2
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${SPDLG_ROOT}
)


set(ZMQ_ROOT ${EXTERNAL_INSTALL_LOCATION}/zmq)
set(ZMQ_INCLUDE_DIR ${ZMQ_ROOT}/include)
set(ZMQ_LIBRARY_DIR ${ZMQ_ROOT}/lib)

ExternalProject_Add(libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ZMQ_ROOT}
)

ExternalProject_Add(cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ZMQ_ROOT}
)

# find_package(ZeroMQ REQUIRED)


# include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
# link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

# set(ZeroMQ_INCLUDE_DIR ${EXTERNAL_INSTALL_LOCATION}/include)
# set(ZeroMQ_LIBRARY ${EXTERNAL_INSTALL_LOCATION}/lib/)

#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# find_package(PkgConfig)
# pkg_check_modules(PC_ZeroMQ QUIET zmq)

# find_path(ZeroMQ_INCLUDE_DIR
#         NAMES zmq.hpp
#         PATHS ${EXTERNAL_INSTALL_LOCATION})

# find_library(ZeroMQ_LIBRARY
#         NAMES zmq
#         PATHS ${EXTERNAL_INSTALL_LOCATION})

# include_directories(SYSTEM ${ZeroMQ_INCLUDE_DIR})

find_package(Threads)



# Logging
# if(NOT TARGET spdlog)
#     find_package(spdlog REQUIRED)
# endif()

# Common
add_subdirectory(common)
include_directories(common/include)

# # Driver
add_subdirectory(driver)
include_directories(driver/include)

# Worker
add_subdirectory(worker)
include_directories(worker/include)

# Applications
add_subdirectory(applications)

