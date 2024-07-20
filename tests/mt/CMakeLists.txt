cmake_minimum_required(VERSION 3.13)

include(utee-test)
	
set(MT_TEST_SOURCES
	ThreadCallbacksTests.cpp
	../test-helpers/src/thread.cpp
)

if(WIN32)
	set(MT_TEST_SOURCES ${MT_TEST_SOURCES}
		ThreadTests.cpp
		../test-helpers/src/thread_win32.cpp
	)
else()
	set(MT_TEST_SOURCES ${MT_TEST_SOURCES}
		../test-helpers/src/thread_unix.cpp
	)
endif()

add_library(mt.tests SHARED ${MT_TEST_SOURCES})
target_link_libraries(mt.tests mt $<TARGET_OBJECTS:mt.thread_callbacks>)
add_utee_test(mt.tests)
