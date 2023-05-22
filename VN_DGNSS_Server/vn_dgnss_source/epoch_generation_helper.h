#pragma once
#include "bkg_data_requestor.h"
#include "create_rtcm_msg.h"
#include "geoid_model_helper.h"
#include "iggtrop_correction_model.h"
#include "rtklib.h"
#include "sat_pos_clk_computer.h"
#include "time_common_func.h"
#include "us_tec_iono_corr_computer.h"
#include "web_data_requestor.h"

struct GnssSystemInfo {
  // System check, sys[0] GPS, sys[1] GAL, sys[2] BDS
  std::vector<bool> sys;
  // Code Freq 1 type, 0 GPS, 1 GAL, 2 BDS
  std::vector<int> code_F1;
  // Code Freq 2 type, 0 GPS, 1 GAL, 2 BDS
  std::vector<int> code_F2;
  GnssSystemInfo() {
    code_F1.resize(3,-1);
    code_F2.resize(3,-1);
  }
};

class EpochGenerationHelper {
 public:
  explicit EpochGenerationHelper(std::vector<double> pos_ecef);
  void GetPppCorrections(BkgDataRequestor *foo_bkg, WebDataRequestor *foo_web, std::ostream &client_log);
  bool ConstructGnssMeas(BkgDataRequestor *foo_bkg, WebDataRequestor *foo_web,
                          std::ostream &rst, const GnssSystemInfo & infor,
                          const IggtropExperimentModel & TropData,
                          int log_count);
  void ComputePhaseWindup(int sys_i, int prn_idx, const std::vector<double> &sat_pos_pretrans);
  void ResetPhaseWindupVec();
  bool SelectSatOrbitCorrection(std::ostream &rst,int prn, int sys,
                  SatOrbitPara &obt_sv, gtime_t &obt_t);
  bool SelectSatClockCorrection(std::ostream &rst,int prn, int sys,
                  SatClockPara &clk_sv, gtime_t &clk_t);
  void SendRtcmMsgToClient(SockRTCM *client_info);
  ~EpochGenerationHelper();

 private:
  std::vector<obsd_t> data{}; // data for RTCM
  int num_sv{};  // Number of satellites available
  std::vector<int> num_in_sys{};
  const std::vector<double> user_pos;  // User position in ECEF (ITRF 2014)
  UsTecCorrData ustec_data;
  VTecCorrection vtec_ssr;
  std::vector<SsrClockCorrEpoch> clock_data;
  std::vector<SsrOrbitCorrEpoch> orbit_data;
  BiasCorrData code_bias;
  BiasCorrData phase_bias;
  SsrCodeBiasEpoch code_bias_ssr;
  SsrPhaseBiasEpoch phase_bias_ssr;
  std::vector<std::vector<double>> phase_windup_track;
  std::vector<GnssEphStruct> eph_data;
  gtime_t gpst_now{};
  int day_of_year{};
  std::vector<double> date_gps;
};
