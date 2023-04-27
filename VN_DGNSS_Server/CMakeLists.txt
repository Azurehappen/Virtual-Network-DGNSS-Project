project(VN_DGNSS_Server)

cmake_minimum_required(VERSION 3.9)

add_subdirectory(common)
add_subdirectory(rtklib)
add_subdirectory(VN_DGNSS_src)
add_subdirectory(requestor)

set(COMPILE_FLAGS "-Wall -Werror -Wpedantic -O3 -pthread")
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES server.cpp)
set(HEADER_FILES server.h)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} VN_DGNSS_src)
target_link_libraries(${PROJECT_NAME} requestor)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")