project(vn_dgnss_source)

cmake_minimum_required(VERSION 3.9)

set(COMPILE_FLAGS "-Wall -Werror -Wpedantic -O3 -pthread")
set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES us_tec_iono_corr_computer.cpp sat_pos_clk_computer.cpp
        geoid_model_helper.cpp epoch_generation_helper.cpp create_rtcm_msg.cpp
        iggtrop_correction_model.cpp
        ssr_vtec_correction_model.cpp beidou_code_correction.cpp)
set(HEADER_FILES us_tec_iono_corr_computer.h sat_pos_clk_computer.h
        geoid_model_helper.h epoch_generation_helper.h create_rtcm_msg.h
        ssr_vtec_correction_model.h
        iggtrop_correction_model.h beidou_code_correction.h)

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} common)
target_link_libraries(${PROJECT_NAME} rtklib)
target_link_libraries(${PROJECT_NAME} requestor)
target_link_libraries(${PROJECT_NAME} curl)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${COMPILE_FLAGS}>")
