#pragma once
#include <iostream>
#include <cstdio>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <vector>
#include <numeric>
#define ELEVMASK 10*3.14159265358980/180

class UsTecIonoCorrComputer {
public:
    /*
     * constructor takes input of USTEC file path
     */
    UsTecIonoCorrComputer(std::vector<std::vector<int>>  USTEC_map_file, std::vector<double>  user_location);

    /*
     * Destroys IonoDelayComputation instance
     */
    ~UsTecIonoCorrComputer();

    /*
     * Parses Ionospheric delay computation parameters
     */
    bool ParseUSTECFile();
    /*
     * ECEF position to lat lon position computation
     */
    void ECEF2LatLon();
    /*
     * Earth model parameter computation
     */
    void Earth2LevelRotation() ;
    /*
     * Elevation and azimuth compuation
     */
    std::vector<double> ElevationAzimuthComputation(const std::vector<double>& sat_pos);

    /*
     * Utility function to do matrix times vector multiplication
     */
    std::vector<double> Matrix_vector_multiplication(const std::vector<std::vector<double>>& A, const std::vector<double>& B);
    /*
     * Pierce point calculation
     */
    std::vector<double> PiercePointCalculation (const double& elevation, const double& azimuth);
    /*
     * Vertical TEC computation
     */
    int VerticalTECComputation(const std::vector<double>& sat_pos);

    /*
     * Slant TEC computation
     */
    double SlantTECComputation (const double& V_TEC);
    /*
     * TEC to iono delay computation
     */
    double TEC2IonoDelayComputation (const double& S_TEC, const double& frequency);

    void GetLatLonHeight(double& lat, double& lon, double& h);

    double GetElevation();
    double GetVTEC();

private:
    std::string USTEC_data_path;
    std::vector<double> user_pos_ecef;
    double user_latitude{};
    double user_longitude{};
    double user_height{};
    std::vector<std::vector<int>> TEC_grid_map;

    // user varying variable
    double elevation{};
    double azimuth{};
    double vertical_TEC{};
    //constant parameters
    const double pi;
    const double Earth_radius;
    const double iono_height;


    // constant earth model parameters
    const double OMEGA_ie; // earth angular rate
    const double semi_major_axis_len; // semi major axis length
    const double semi_minor_axis_len; // semi minor axis length
    const double flat_e; // flatness of ellipsoid
    const double ecc_e; // first eccentricity of ellipsoid
    std::vector<std::vector<double>> R_e2l; // rotation matrix for earth to level

    //constant data parameters
    const double min_lat = 10;
    const double max_lat = 60;
    const double min_lon = -150;
    const double max_lon = -50;


};
