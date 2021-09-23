#pragma once
#include "IonoDelay.h"
#include "SatPosClkComp.h"
#include "data2rtcm.h"
#include "geoid.h"
#include "requestor_web.h"
#include "requestor_BKG.h"
#include "rtklib.h"
#include "timecmn.h"
#include "IGGtrop.h"

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
  void getcorrdata(requestor_BKG *foo_bkg, requestor_web *foo_web, std::ostream &rst);
  bool computemeasrements(requestor_BKG *foo_bkg, requestor_web *foo_web,
                          std::ostream &rst, const sys_infor& infor,
                          const IGGexpModel& TropData,
                          std::vector<std::vector<double>> &phw_track,
                          int log_count);
  bool orbit_pick(std::ostream &rst,int prn, int sys,
                  orbit_element &obt_sv, gtime_t &obt_t);
  bool clock_pick(std::ostream &rst,int prn, int sys,
                  clock_element &clk_sv, gtime_t &clk_t);
  void getgpstnow();
  void sendRTCM(SockRTCM *client_info);
  ~datagent();

 private:
  std::vector<obsd_t> data{}; // data for RTCM
  int num_sv{};  // Number of satellites available
  std::vector<int> num_in_sys{};
  const std::vector<double> user_pos;  // User position in ECEF (ITRF 2014)
  ustec ustec_data;
  vtec_t vtec_CNE;
  std::vector<ssr_clock> clock_data;
  std::vector<ssr_orbit> orbit_data;
  code_bias_strc code_bias;
  cbias_ssr code_bias_ssr;
  pbias_ssr phase_bias_ssr;
  //std::vector<gps_phase_bias> phase_bias;
  std::vector<eph_struct> eph_data;
  gtime_t gpst_now{};
  int doy{};  // day of year
  std::vector<double> date_gps;
  // ofstream rst;
};
