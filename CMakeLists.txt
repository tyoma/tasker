cmake_minimum_required(VERSION 3.13)

project(tasker)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/libraries/utee/make/build.props)

option(TASKER_NO_TESTS "Do not build test modules." OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)

include_directories(${CMAKE_CURRENT_LIST_DIR})

if (UNIX)
	set(MP_MT_GENERIC 1)
	add_definitions(-DMP_MT_GENERIC)
endif()	
add_compile_options("$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fPIC;-Wpedantic;-Wall;-std=c++11>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:-W4>")
add_link_options("$<$<CXX_COMPILER_ID:GNU>:-Wl,--no-undefined;-Wl,--gc-sections>")
							
add_subdirectory(src/mt)
add_subdirectory(src/tasker)

target_include_directories(mt INTERFACE .)
target_include_directories(tasker INTERFACE .)

if (NOT TASKER_NO_TESTS)
	if (NOT TARGET utee)
		set(UTEE_NO_TESTS ON)
		add_subdirectory(libraries/utee)
	endif()
	add_subdirectory(tests/mt)
	add_subdirectory(tests/tasker)
	
	add_utee_test(mt.tests)
	add_utee_test(tasker.tests)
endif()
