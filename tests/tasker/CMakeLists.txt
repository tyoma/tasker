cmake_minimum_required(VERSION 3.13)

include(utee-test)

set(TASKER_TEST_SOURCES
	AsyncResultTests.cpp
	PrivateQueueTests.cpp
	ThreadQueueTests.cpp
	TaskNodeTests.cpp
	TaskTests.cpp
	TaskQueueTests.cpp
	../test-helpers/src/thread.cpp
)

if (WIN32)
	set(TASKER_TEST_SOURCES ${TASKER_TEST_SOURCES}
		helpers_win32.cpp
		UIQueueTests.cpp
		../test-helpers/src/thread_win32.cpp
	)
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(TASKER_TEST_SOURCES ${TASKER_TEST_SOURCES}
		helpers_macos.mm
		UIQueueTests.cpp
	)
elseif (ANDROID)
	set(TASKER_TEST_SOURCES ${TASKER_TEST_SOURCES}
		helpers_android.cpp
		UIQueueTests.cpp
	)	
endif()

if (NOT WIN32)
	set(TASKER_TEST_SOURCES ${TASKER_TEST_SOURCES}
		helpers_generic.cpp
		../test-helpers/src/thread_unix.cpp
	)
endif()

add_library(tasker.tests SHARED ${TASKER_TEST_SOURCES})
target_link_libraries(tasker.tests tasker mt $<TARGET_OBJECTS:mt.thread_callbacks>)
add_utee_test(tasker.tests)
