cmake_minimum_required(VERSION 3.16)
project(web_server C)

set(CMAKE_C_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wpedantic -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

find_package (Threads REQUIRED)

add_executable(web_server web_server.c)

target_link_libraries (web_server Threads::Threads)
