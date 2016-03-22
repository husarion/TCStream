cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(tcstream)
find_package(Threads)

if(X86)
	set(CMAKE_CXX_FLAGS "-m32 -std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "-m32")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "")
endif()

option(PORT "port")

include_directories(../../port/hframework)
include_directories(../../)

if(NOT HFRAMEWORK_PATH)
	set(HFRAMEWORK_PATH "${CURRENT_DIR}/../hFramework")
endif()

include(${HFRAMEWORK_PATH}/hFramework.cmake)

# add_definitions(-DTCSTREAM_DEBUG)

add_hexecutable(test main.cpp ../../tcstream.cpp)
