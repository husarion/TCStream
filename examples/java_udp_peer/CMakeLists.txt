cmake_minimum_required(VERSION 2.6)
get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH) # for cmake before 2.8.3

project(Example_UDP)
find_package(Threads)
find_package(JNI)
find_package(Java)
include(UseJava)

if(X86)
	set(CMAKE_CXX_FLAGS "-m32 -std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "-m32")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall")
	set(CMAKE_EXE_LINKER_FLAGS "")
endif()

add_jar(Example_UDP SOURCES Example_UDP.java TCStream.java ENTRY_POINT Example_UDP)
