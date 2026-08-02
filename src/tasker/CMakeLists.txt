cmake_minimum_required(VERSION 3.13)

set(TASKER_SOURCES
	no_queue.cpp
	private_queue.cpp
	task_queue.cpp
	thread_queue.cpp
)

if (WIN32)
	set(TASKER_SOURCES ${TASKER_SOURCES}
		ui_queue_win32.cpp
	)
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(TASKER_SOURCES ${TASKER_SOURCES}
		ui_queue_macos.mm
	)
elseif (ANDROID)
	set(TASKER_SOURCES ${TASKER_SOURCES}
		ui_queue_android.cpp
	)
endif()

add_library(tasker STATIC ${TASKER_SOURCES})
target_compile_definitions(tasker PUBLIC $<$<BOOL:${TASKER_USE_STD_MT}>:TASKER_USE_STD_MT>)

if (APPLE)
	target_link_libraries(tasker "-framework Foundation -framework Cocoa")
elseif (ANDROID) 
	target_link_libraries(tasker log android)
endif()
