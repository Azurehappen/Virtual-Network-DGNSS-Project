#pragma once
#include "IGGtrop.h"
#include "IonoDelay.h"
#include "SatPosClkComp.h"
#include "bkg_data_requestor.h"
#include "data2rtcm.h"
#include "geoid.h"
#include "rtklib.h"
#include "time_common_func.h"
#include "web_data_requestor.h"

struct sys_infor {
  // System check, sys[0] GPS, sys[1] GAL, sys[2] BDS
  std::vector<bool> sys;
  // Code Freq 1 type, 0 GPS, 1 GAL, 2 BDS
  std::vector<int> code_F1;
  // Code Freq 2 type, 0 GPS, 1 GAL, 2 BDS
  std::vector<int> code_F2;
  sys_infor() {
    code_F1.resize(3,-1);
    code_F2.resize(3,-1);
  }
};

class datagent {
 public:
  explicit datagent(std::vector<double> pos_ecef);
  void getcorrdata(BkgDataRequestor *foo_bkg, WebDataRequestor *foo_web, std::ostream &rst);
  bool computemeasrements(BkgDataRequestor *foo_bkg, WebDataRequestor *foo_web,
                          std::ostream &rst, const sys_infor& infor,
                          const IGGexpModel& TropData,
                          std::vector<std::vector<double>> &phw_track,
                          int log_count);
  bool orbit_pick(std::ostream &rst,int prn, int sys,
                  SatOrbitPara &obt_sv, gtime_t &obt_t);
  bool clock_pick(std::ostream &rst,int prn, int sys,
                  SatClockPara &clk_sv, gtime_t &clk_t);
  void getgpstnow();
  void sendRTCM(SockRTCM *client_info);
  ~datagent();

 private:
  std::vector<obsd_t> data{}; // data for RTCM
  int num_sv{};  // Number of satellites available
  std::vector<int> num_in_sys{};
  const std::vector<double> user_pos;  // User position in ECEF (ITRF 2014)
  UsTecCorrData ustec_data;
  VTecCorrection vtec_CNE;
  std::vector<SsrClockCorrEpoch> clock_data;
  std::vector<SsrOrbitCorrEpoch> orbit_data;
  BiasCorrData code_bias;
  BiasCorrData phase_bias;
  SsrCodeBiasEpoch code_bias_ssr;
  SsrPhaseBiasEpoch phase_bias_ssr;
  //std::vector<gps_phase_bias> phase_bias;
  std::vector<GnssEphStruct> eph_data;
  gtime_t gpst_now{};
  int doy{};  // day of year
  std::vector<double> date_gps;
  // ofstream rst;
};
