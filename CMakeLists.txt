cmake_minimum_required(VERSION 3.16)
project(Assignment_2 C)

set(CMAKE_C_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wpedantic -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

find_package (Threads REQUIRED)

add_executable(Assignment_2
        httpserver.c
        queue.c
        queue.h)

target_link_libraries (Assignment_2 Threads::Threads)

add_executable(current
        logtest.c
        queue.c
        queue.h)

target_link_libraries (current Threads::Threads)
