cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(tcstream)
find_package(Threads)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

add_definitions(-DTCSTREAM_DEBUG=1)

include_directories(${CURRENT_DIR}/../kdutils)
include_directories(${CURRENT_DIR}/../..)
include_directories(${CURRENT_DIR}/../../port/linux)

link_directories(${CURRENT_DIR}/../..)
link_directories(${CURRENT_DIR}/../kdutils)

add_executable(linux_serial_peer linux_serial_peer.cpp ../../tcstream.cpp ../kdutils/kdutils.cpp)
target_link_libraries(linux_serial_peer pthread util)

