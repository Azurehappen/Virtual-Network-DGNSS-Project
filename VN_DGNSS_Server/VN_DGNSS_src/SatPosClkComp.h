#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "rtklib.h"

class SatPosClkComp {
public:
  struct Ephemeris {
    // variable parameters
    int prn;
    size_t weekNo{};      // GPS Week #
    double svAcc{};  // SV accuracy (meters)
    int svH{};    // SV health
    size_t IODC{};        // IODC Issue of Data, Clock
    size_t IODE{};        // IODE Issue of Data, Ephemeris
    double T_GD{};      // TGD (seconds) 1. BDS TGD1 B1/B3 2. GAL BGD E5a/E1
    double T_GD2{};     // 1. BDS TGD2 B2/B3 2. GAL BGD E5b/E1
    gtime_t t_oc{};     // All system t_oc represented in GPS time in this struct
    gtime_t t_oe{};
    double toes{};      // Toes Time of Ephemeris, sec of week
    double a_f0{};      // SV clock bias (seconds)
    double a_f1{};      // SV clock drift (sec/sec)
    double a_f2{};      // SV clock drift rate (sec/sec2)
    double M_0{};       // M0 (radians)
    double e{};         // e Eccentricity
    double Delta_n{};   // Delta n (radians/sec)
    double sqrtA{};     // sqrt(A) (sqrt(m))
    double Omega_0{};   // OMEGA0 (radians)
    double i_0{};       // i0 (radians)
    double omega{};     // omega (radians)
    double OmegaDot{};  // OMEGA DOT (radians/sec)
    double C_uc{};      // Cuc (radians)
    double C_us{};      // Cus (radians)
    double C_rc{};      // Crc (meters)
    double C_rs{};      // Crs (meters)
    double C_ic{};      // Cic (radians)
    double C_is{};      // Cis (radians)
    double IDOT{};      // IDOT (radians/sec)
    int data_src{};    // GAL/BDS Data sources
    Ephemeris() {
      prn = -1;
      IODC = -1;
      IODE = 9999;
    }
  };
  // instance of the structure
  Ephemeris eph_0;

  /*
   * Constructor for ephemeris usage class
   */
  SatPosClkComp(gtime_t rcv_t, std::vector<double> dX, std::vector<double> dV,
                gtime_t  orb_corr_time, std::vector<double> dt_corr,
                gtime_t  clk_corr_time, Ephemeris eph_data_0, int sys);

  // limit gps time
  static double limit_gpstime(double t_gps);

  // satellite clock bias computation with ephemeris data
  void SatClkComputation();

  // precise satellite clock computation
  void PreciseSatClkComputation();

  // satellite position computation with ephemeris data
  void SatEphPosComputation();

  // function computing kepler anomaly
  static double KeplerAnomaly(double ecc, double M_k);

  // satellite velocity computation with ephemeris data using Farrell(C.4.1)
  void SatEphVelComputation();

  // satellite velocity computation with ephemeris data using Remondi
  void SatEphVelComputationUpdated();

  // earth rotation correction
  void EarthRotationCorrection(std::vector<double> &x_pos) const;

  // RAC to ECEF transformation function
  static std::vector<double> RAC2ECEF(std::vector<double> dp_RAC,
                               std::vector<double> p_ecef,
                               std::vector<double> v_ecef);

  // precise orbit computation
  void PreciseSatPosComputation();

  // Sagnac correction
  double Sagnac(std::vector<double> user_pos);

  // Optimize the satellite pseudo propagation time with client position.
  void PropTimeOptm(std::vector<double> user_pos);

  // Compute precise or
  void ComputePreciseOrbitClockCorrection();

  // get orbit position
  std::vector<double> GetSatPos();

  // get precise orbit position
  std::vector<double> GetPreciseSatPos();
  std::vector<double> GetPreSatPosAtTranst();
  // get sat clock bias
  double GetClock();   // clock corr

  ~SatPosClkComp();
  //{eph_0.clear();}
private:
  int sys;
  // satellite position computation variables
  double E_k{};
  double propagation_time{};
  gtime_t received_time;
  gtime_t transmit_time{};

  // precise ephemeris data
  std::vector<double> dX_rac;
  std::vector<double> dV_rac;
  gtime_t ssr_orbit_time;

  // precise clock data
  std::vector<double> dt_corr;
  gtime_t ssr_clock_time;

  // satellite clock computation
  double dt_clk{};
  double dt_R{};
  double dt_g1{};
  double dt_g2{};
  double dt_clk_precise{};

  // satellite position computation
  std::vector<double> sat_pos_ecef;
  std::vector<double> sat_vel_ecef;
  std::vector<double> sat_pos_ecef_precise;
  std::vector<double>
          sat_pos_pre_trans;  // sat position at transmit time
  // vector<double> sat_pos_ecef_precise;

  // miscellonius variables
  double A{}, n{}, sEk{}, cEk{}, r_k{}, cvk{}, svk{}, s2Phik{}, c2Phik{}, cuk{}, suk{}, c2uk{}, s2uk{},
          cik{}, sik{}, sOmeg{}, cOmeg{}, x_k_prime{}, y_k_prime{}, x_k{}, y_k{}, z_k{};

  // constant parameters
  const double OmegaDot_e;
  const double lambda_L1;
  const double lambda_L2;
  const double mu;
  const double mask_ang_rad;
  const double F;
};
