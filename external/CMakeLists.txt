cmake_minimum_required(VERSION 3.12)
project(EXTERNAL_PROJECTS)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/catch2/contrib")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/catch2/contrib" PARENT_SCOPE)

set(EVENT__DISABLE_TESTS ON)
add_subdirectory(libevent)
add_library(libevent INTERFACE)
target_include_directories(libevent INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libevent/include ${CMAKE_CURRENT_BINARY_DIR}/libevent/include)
target_link_libraries(libevent INTERFACE event_core event_pthreads)

if (NOT TARGET Catch2::Catch2)
    add_subdirectory(catch2)
endif()
    
if (PTL_BUILD_UNITTEST)
    add_library(ptl_unittest_runner
        unittest_runner.cpp
    )
    target_link_libraries(ptl_unittest_runner PUBLIC ptl Catch2::Catch2)
    target_compile_definitions(ptl_unittest_runner PUBLIC -DBUILD_UNITTEST)

    function(add_ptl_unittest TARGET)
        include(CTest)
        include(Catch)

        cmake_parse_arguments(PARSE_ARGV 1 UT "" "" "SOURCES;LIBS")
        add_executable(${TARGET} ${UT_SOURCES})
        if (ENABLE_ASAN)
            target_compile_options(${TARGET} PRIVATE -fsanitize=address)
            target_link_libraries(${TARGET} PRIVATE -fsanitize=address)
        endif()
        target_link_libraries(${TARGET} PRIVATE ptl_unittest_runner Threads::Threads)
        foreach(UT_LIB ${UT_LIBS})
            target_link_libraries(${TARGET} PRIVATE ${UT_LIB})
        endforeach()

        target_compile_options(${TARGET} PRIVATE -march=native)
        target_compile_features(${TARGET} PRIVATE cxx_std_20)
        message("catch: ${TARGET}")
        catch_discover_tests(${TARGET})
    endfunction()
else()
    function(add_ptl_unittest TARGET)
    endfunction()
endif()



include(Catch)
