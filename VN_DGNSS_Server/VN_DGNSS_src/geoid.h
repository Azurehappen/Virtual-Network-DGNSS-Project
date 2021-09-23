#pragma once
#include "SatPosClkComp.h"

class geoid {
public:
    double geoidh(double geo_lat,double geo_lon);
    
private:
    double interpb(double *y, double a, double d);
    static const double range[];
    static const float geoidval[361][181];
};

