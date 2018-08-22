cmake_minimum_required(VERSION 3.9)
project(pressure_test C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=c99 -pthread")
add_executable(pressure_test main.c threadpool.c threadpool.h pressureTest.h pressureTest.c)