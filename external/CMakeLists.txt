cmake_minimum_required(VERSION 3.12)
project(EXTERNAL_PROJECTS)
add_subdirectory(catch2)
add_subdirectory(libevent)

add_library(unittest
    unittest_runner.cpp
)
target_link_libraries(unittest PUBLIC Catch2::Catch2)
target_compile_definitions(unittest PUBLIC -DBUILD_UNITTEST)
if (${BUILD_COROUTINE})
	target_compile_options(unittest PUBLIC -stdlib=libc++)
    target_compile_features(unittest PUBLIC cxx_std_20)
	target_compile_options(unittest PUBLIC -stdlib=libc++)
else()
    target_compile_features(unittest PUBLIC cxx_std_17)
endif()

add_library(libevent INTERFACE)
target_include_directories(libevent INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libevent/include ${CMAKE_CURRENT_BINARY_DIR}/libevent/include)
target_link_libraries(libevent INTERFACE event_core event_pthreads)

include(Catch)
