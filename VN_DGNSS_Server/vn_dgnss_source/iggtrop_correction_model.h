#ifndef VN_DGNSS_SERVER_IGGTROP_CORRECTION_MODEL_H
#define VN_DGNSS_SERVER_IGGTROP_CORRECTION_MODEL_H

#pragma once
#include "sat_pos_clk_computer.h"
#include <fstream>

struct IggtropExperimentModel {
  std::vector<std::vector<std::vector<std::vector<float>>>> data;
};
IggtropExperimentModel GetIggtropCorrDataFromFile(const std::string& file);

class IggtropCorrectionModel {
public:
  double IGGtropdelay(
      double uLon,double uLat,double uH,int doy, double elev,
      const std::vector<std::vector<std::vector<std::vector<float>>>>& data);
private:
  static int fix(double val);
};

#endif // VN_DGNSS_SERVER_IGGTROP_CORRECTION_MODEL_H
