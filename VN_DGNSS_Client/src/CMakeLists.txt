cmake_minimum_required(VERSION 3.9)
project(src)

IF (WIN32)
    # set stuff for windows
    add_library(
            src
            "connection_win.cpp"
            "connection_win.h"
            "serial_read_win.cpp"
            "serial_read_win.h"
            "utility.h"
            "utility.cpp")
ELSE()
    set(CMAKE_CXX_FLAGS "-pthread")
    set(COMPILE_FLAGS "-std=c++17 -Wall -Werror -Wpedantic -O3 -pthread")
    # set stuff for linux
    add_library(
            src
            "connection.cpp"
            "connection.h"
            "serial_read.cpp"
            "serial_read.h"
            "utility.h"
            "utility.cpp")
    find_package(Threads REQUIRED)
    target_link_libraries(src PUBLIC Threads::Threads)
ENDIF()
