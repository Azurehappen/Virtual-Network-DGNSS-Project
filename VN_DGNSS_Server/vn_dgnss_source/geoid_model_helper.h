#pragma once
#include "sat_pos_clk_computer.h"

class GeoidModelHelper {
public:
    double geoidh(double geo_lat,double geo_lon);
    
private:
    double interpb(double *y, double a, double d);
    static const double range[];
    static const float geoidval[361][181];
};

