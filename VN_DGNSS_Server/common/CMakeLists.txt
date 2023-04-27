project(common)

cmake_minimum_required(VERSION 3.9)

set(COMPILE_FLAGS "-std=c++17 -Wall -Werror -Wpedantic -O3")
set(SOURCE_FILES time_common_func.cpp)
set(HEADER_FILES data_struct.h time_common_func.h constants.h)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} rtklib)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")