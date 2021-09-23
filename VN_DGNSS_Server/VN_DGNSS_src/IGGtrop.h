#pragma once
#include "SatPosClkComp.h"
#include <fstream>

#ifndef WADGNSS_SERVER_IGGTROP_H
#define WADGNSS_SERVER_IGGTROP_H

struct IGGexpModel{
  std::vector<std::vector<std::vector<std::vector<float>>>> data;
};
IGGexpModel getIGGtropData(const std::string& file);
class IGGtrop {
public:
  double IGGtropdelay(
      double uLon,double uLat,double uH,int doy, double elev,
      const std::vector<std::vector<std::vector<std::vector<float>>>>& data);
private:
  static int fix(double val);
};

#endif // WADGNSS_SERVER_IGGTROP_H
