cmake_minimum_required(VERSION 3.9)
project(VN_DGNSS_Client)
add_subdirectory(src)
IF (WIN32)
    # set stuff for windows
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../../app/)
    add_executable(
            VN_DGNSS_Client
            "client_main_win.cpp")
ELSE()
    set(COMPILE_FLAGS "-std=c++17 -Wall -Werror -Wpedantic -O3 -pthread")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin/)
    # set stuff for linux
    add_executable(
            VN_DGNSS_Client
            "client_main.cpp")
    target_compile_options(VN_DGNSS_Client PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")
ENDIF()
target_link_libraries(VN_DGNSS_Client PUBLIC src)
target_include_directories(VN_DGNSS_Client PUBLIC src)
