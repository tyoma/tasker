cmake_minimum_required(VERSION 3.13)

project(tasker)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/libraries/utee/make/build.props)

option(TASKER_NO_TESTS "Do not build test modules." OFF)
option(TASKER_USE_STD_MT "Use std-provided multi-threading implementation." ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)

set(MSVC_COMPILER_OPTIONS -wd4503;-wd4996;-wd4510;-wd4610;-wd4512;-wd4312;-wd4481;-wd4355)
set(MSVC_COMPILER_OPTIONS ${MSVC_COMPILER_OPTIONS} -MP;-W4;-Zi)
set(MSVC_COMPILER_OPTIONS_DEBUG ${MSVC_COMPILER_OPTIONS})
set(MSVC_COMPILER_OPTIONS_RELEASE_MAX ${MSVC_COMPILER_OPTIONS} -Ox;-Ob2;-Oi;-Ot;-Oy)

add_compile_options(
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:${MSVC_COMPILER_OPTIONS_DEBUG}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release,RelWithDebInfo>:${MSVC_COMPILER_OPTIONS_RELEASE_MAX}>>"
	"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fPIC;-Wpedantic;-Wall;-Wno-unknown-pragmas;-std=c++11>"
	"$<$<PLATFORM_ID:Linux>:-pthread>"
)

add_link_options(
	"$<$<CXX_COMPILER_ID:MSVC>:-DEBUG:FULL>"
	"$<$<CXX_COMPILER_ID:GNU>:-Wl,--no-undefined;-Wl,--gc-sections>"
)

add_subdirectory(src/mt)
add_subdirectory(src/tasker)

if (NOT TASKER_NO_TESTS)
	if (NOT TARGET utee)
		set(UTEE_NO_TESTS ON)
		add_subdirectory(libraries/utee)
	endif()
	add_subdirectory(tests/mt)
	add_subdirectory(tests/tasker)
	add_subdirectory(benchmarks)
	
	add_utee_test(mt.tests)
	add_utee_test(tasker.tests)
endif()
