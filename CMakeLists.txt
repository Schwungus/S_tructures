cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(S_tructures
    VERSION 0.1.0
    LANGUAGES C)

add_library(S_tructures INTERFACE)
target_include_directories(S_tructures
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(S_TRUCTURES_BUILD_TESTS "Build the test executable?" ON)
if(S_TRUCTURES_BUILD_TESTS)
    add_executable(S_tructuresTest
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tests.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/static.c)
    target_link_libraries(S_tructuresTest S_tructures)
    if(MSVC)
        target_compile_options(S_tructuresTest
            PRIVATE /fsanitize=address)
    endif()
endif()
