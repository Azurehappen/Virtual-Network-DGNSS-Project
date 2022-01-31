project(VN_DGNSS_src)

cmake_minimum_required(VERSION 3.9)

set(COMPILE_FLAGS "-std=c++17 -Wall -Werror -Wpedantic -O3 -pthread")
set(SOURCE_FILES IonoDelay.cpp SatPosClkComp.cpp
        geoid.cpp timecmn.cpp datagent.cpp data2rtcm.cpp SSR_VTEC.cpp)
set(HEADER_FILES IonoDelay.h SatPosClkComp.h
        geoid.h timecmn.h datagent.h data2rtcm.h SSR_VTEC.h IGGtrop.cpp IGGtrop.h)

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} rtklib)
target_link_libraries(${PROJECT_NAME} requestor)
target_link_libraries(${PROJECT_NAME} curl)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")
