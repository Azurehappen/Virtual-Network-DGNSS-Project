project(rtklib)

cmake_minimum_required(VERSION 3.9)

set(COMPILE_FLAGS "-Wall -Werror -Wpedantic -O3")
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES cmnfunc.cpp rtcm.cpp rtcm3e.cpp phase_windup.cpp)
set(HEADER_FILES rtklib.h)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")
