#ifndef VN_DGNSS_SERVER_CONSTANTS_H
#define VN_DGNSS_SERVER_CONSTANTS_H
#pragma once
#include <map>

#include "rtklib.h"

#define VN_CODE_GPS_NONE 0
#define VN_CODE_GPS_C1C 1
#define VN_CODE_GPS_C1W 2
#define VN_CODE_GPS_C2C 3
#define VN_CODE_GPS_C2W 4
#define VN_CODE_GPS_C2L 5
#define MAX_VN_CODE_GPS 6

#define VN_CODE_GAL_NONE 0
#define VN_CODE_GAL_C1C 1
#define VN_CODE_GAL_C1X 2
#define VN_CODE_GAL_C6C 3
#define VN_CODE_GAL_C5Q 4
#define VN_CODE_GAL_C5X 5
#define VN_CODE_GAL_C7Q 6
#define VN_CODE_GAL_C7X 7
#define MAX_VN_CODE_GAL 8

#define VN_CODE_BDS_NONE 0
#define VN_CODE_BDS_C2I 1
#define VN_CODE_BDS_C6I 2
#define VN_CODE_BDS_C7 3
#define MAX_VN_CODE_BDS 4

#define MAX_CODE_ELEMENTS 10

#define VN_MAX_NUM_OF_EPH_EPOCH 10

namespace GnssConstants {
// Nested map to store wavelength values for each signal
using VnCodeToWavelenth = std::map<int, double>;

// Map to store signal wavelengths for each GNSS system
const std::map<int, VnCodeToWavelenth> SysCodeToWavelength = {
    {SYS_GPS,  // GPS
     {
         {VN_CODE_GPS_C1C, CLIGHT / FREQL1},
         {VN_CODE_GPS_C1W, CLIGHT / FREQL1},
         {VN_CODE_GPS_C2C, CLIGHT / FREQL2},
         {VN_CODE_GPS_C2L, CLIGHT / FREQL2},
         {VN_CODE_GPS_C2W, CLIGHT / FREQL2},
     }},
    {SYS_GAL,  // GAL
     {
         {VN_CODE_GAL_C1C, CLIGHT / FREQL1},
         {VN_CODE_GAL_C1X, CLIGHT / FREQL1},
         {VN_CODE_GAL_C6C, CLIGHT / FREQE6},
         {VN_CODE_GAL_C5Q, CLIGHT / FREQL5},
         {VN_CODE_GAL_C5X, CLIGHT / FREQL5},
         {VN_CODE_GAL_C7Q, CLIGHT / FREQE5b},
         {VN_CODE_GAL_C7X, CLIGHT / FREQE5b},
     }},
    {SYS_CMP,  // BDS
     {
         {VN_CODE_BDS_C2I, CLIGHT / FREQ1_CMP},
         {VN_CODE_BDS_C6I, CLIGHT / FREQ3_CMP},
         {VN_CODE_BDS_C7, CLIGHT / FREQ2_CMP},
     }},
};

}  // namespace GnssConstants
#endif  // VN_DGNSS_SERVER_CONSTANTS_H
