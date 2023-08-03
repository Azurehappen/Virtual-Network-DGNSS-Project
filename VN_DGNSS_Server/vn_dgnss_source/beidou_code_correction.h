#ifndef VN_DGNSS_SERVER_BEIDOU_CODE_CORRECTION_H
#define VN_DGNSS_SERVER_BEIDOU_CODE_CORRECTION_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "constants.h"
/*
 * Elevation-dependent correction values for Beidou code measurements
 */
class beidouCodeCorrection {
 public:
  static double computeBdsCodeCorr(int prn, double ele_deg, int bds_code_type);
};

#endif  // VN_DGNSS_SERVER_BEIDOU_CODE_CORRECTION_H
