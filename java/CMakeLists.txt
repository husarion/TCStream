cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(TCStream)
find_package(JNI)

if(X86)
	set(CMAKE_CXX_FLAGS "-m32 -std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "-m32")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "")
endif()

include_directories(${JNI_INCLUDE_DIRS})
include_directories(..)
include_directories(../port/linux)

add_library(tcstream SHARED ../tcstream.cpp TCStream.cpp)

if(DEBUG)
	set_target_properties(tcstream PROPERTIES COMPILE_FLAGS "-g" LINK_FLAGS "-g")
endif()
