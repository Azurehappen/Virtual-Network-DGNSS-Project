#ifndef VN_DGNSS_SERVER_DATA_STRUCT_H
#define VN_DGNSS_SERVER_DATA_STRUCT_H

#include <cstdio>

#include "rtklib.h"
namespace satstruct {

struct Ephemeris {
  // variable parameters
  int prn;
  size_t weekNo{};  // GPS Week #
  double svAcc{};   // SV accuracy (meters)
  int svH{};        // SV health
  size_t IODC{};    // IODC Issue of Data, Clock
  size_t IODE{};    // IODE Issue of Data, Ephemeris
  double T_GD{};    // TGD (seconds) 1. BDS TGD1 B1/B3 2. GAL BGD E5a/E1
  double T_GD2{};   // 1. BDS TGD2 B2/B3 2. GAL BGD E5b/E1
  gtime_t t_oc{};   // All system t_oc represented in GPS time in this struct
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
  int data_src{};     // GAL/BDS Data sources
  Ephemeris() {
    prn = -1;
    IODC = -1;
    IODE = 9999;
  }
};
}  // namespace satstruct

#endif  // VN_DGNSS_SERVER_DATA_STRUCT_H
