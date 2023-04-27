project(requestor)

cmake_minimum_required(VERSION 3.9)

find_package(Threads REQUIRED)

set(COMPILE_FLAGS "-Wall -Werror -Wpedantic -O3 -pthread")
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES web_data_requestor.cpp bkg_data_requestor.cpp)
set(HEADER_FILES web_data_requestor.h bkg_data_requestor.h)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
find_package(ZLIB)
target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} common)
target_link_libraries(${PROJECT_NAME} curl)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")


