cmake_minimum_required(VERSION 3.13)

set(MT_SOURCES hybrid_event.cpp)

if(MP_MT_GENERIC)
	add_definitions(-DMP_MT_GENERIC)
	set(MT_SOURCES ${MT_SOURCES}
		event_generic.cpp
	)
elseif(WIN32)
	set(MT_SOURCES ${MT_SOURCES}
		atomic_win32.cpp
		event_win32.cpp
		intrinsic_win32.cpp
		mutex_win32.cpp
		thread_win32.cpp
		tls_win32.cpp
	)
endif()

if(WIN32)
	set(MT_SOURCES ${MT_SOURCES}
		tls_win32.cpp
	)
	set(MT_THREAD_SOURCES
		thread_callbacks_win32.cpp
	)
elseif(UNIX)
	set(MT_SOURCES ${MT_SOURCES}
		intrinsic_unix.cpp
		tls_pthreads.cpp
	)
	set(MT_THREAD_SOURCES
		thread_callbacks_unix.cpp
	)
endif()

add_library(mt STATIC ${MT_SOURCES})
add_library(mt.thread_callbacks OBJECT ${MT_THREAD_SOURCES})
