cmake_minimum_required(VERSION 3.16)
project(httpserver C)

set(CMAKE_C_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wpedantic -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

find_package (Threads REQUIRED)

add_executable(httpserver httpserver.c http.h http.c threads.h threads.c)
target_link_libraries (httpserver Threads::Threads)

add_executable(tests tests.c http.h http.c)
target_link_libraries (tests Threads::Threads /usr/lib/libcheck.so)
