cmake_minimum_required(VERSION 3.13)
project(PTL)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/catch2/contrib")

add_library(ptl src/scope_guard.cpp)
target_include_directories(ptl PUBLIC ${PSL_SOURCE_DIR}/include)
target_compile_features(ptl PUBLIC cxx_std_17)


add_subdirectory(external)
add_subdirectory(test)