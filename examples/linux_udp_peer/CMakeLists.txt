cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(tcstream)
find_package(Threads)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")

include_directories(${CURRENT_DIR}/../kdutils)
include_directories(${CURRENT_DIR}/../..)
include_directories(${CURRENT_DIR}/../../port/linux)

link_directories(${CURRENT_DIR}/../..)
link_directories(${CURRENT_DIR}/../kdutils)

add_executable(linux_udp_peer linux_udp_peer.cpp ../../tcstream.cpp ../kdutils/UdpSocket.cpp ../kdutils/kdutils.cpp)
target_link_libraries(linux_udp_peer pthread util)

