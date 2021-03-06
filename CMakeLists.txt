cmake_minimum_required(VERSION 3.13)
project(PTL)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/catch2/contrib")

set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE STRING "Override CXX Flags" FORCE)

set(PTL_IS_SUBPROJECT TRUE)
set(PTL_ON_IF_SUBPROJECT ON)
set(PTL_OFF_IF_SUBPROJECT OFF)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	set(PTL_IS_SUBPROJECT FALSE)
	set(PTL_ON_IF_SUBPROJECT OFF)
	set(PTL_OFF_IF_SUBPROJECT ON)
endif()

option(PTL_BUILD_UNITTEST "Build PTL Unittests" ${PTL_OFF_IF_SUBPROJECT})
option(PTL_BUILD_COROUTINES "Build PTL Coroutines" ON)


find_package(LibEv REQUIRED)
find_package(Threads REQUIRED)

set(BUILD_COROUTINE 0)
if (PTL_BUILD_COROUTINES STREQUAL "ON")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		string(REPLACE "." ";" CLANG_VERSION ${CMAKE_CXX_COMPILER_VERSION})
		list(GET CLANG_VERSION 0 CLANG_VERSION_MAJOR)
		list(GET CLANG_VERSION 1 CLANG_VERSION_MINOR)
		list(GET CLANG_VERSION 2 CLANG_VERSION_PATCH)

		if (${CLANG_VERSION_MAJOR} GREATER_EQUAL 9)
			set(BUILD_COROUTINE 1)
		endif()
	endif()
endif()

set(PTL_SOURCES
	src/scope_guard.cpp
	src/assert.cpp
	src/io_service.cpp
)

if (BUILD_COROUTINE)
list(APPEND PTL_SOURCES src/socket.cpp)
endif()

add_library(ptl
	${PTL_SOURCES}
)

target_include_directories(ptl PUBLIC ${PTL_SOURCE_DIR}/include)
target_link_libraries(ptl PUBLIC ${LIBEV_LIBRARIES})
target_include_directories(ptl PUBLIC ${LIBEV_INCLUDE_DIR})
if (${BUILD_COROUTINE})
	target_compile_options(ptl PUBLIC -fcoroutines-ts -stdlib=libc++)
	target_compile_definitions(ptl PUBLIC BUILD_COROUTINE)
	target_compile_features(ptl PUBLIC cxx_std_20)
	target_link_options(ptl PUBLIC -stdlib=libc++)
else()
	target_compile_features(ptl PUBLIC cxx_std_17)
endif()


if (${PTL_BUILD_UNITTEST})
	include(CTest)
	enable_testing()
endif()

add_subdirectory(external)
add_subdirectory(test)

