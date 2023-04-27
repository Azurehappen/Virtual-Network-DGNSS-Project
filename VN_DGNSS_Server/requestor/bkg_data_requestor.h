#ifndef VN_DGNSS_SERVER_BKG_DATA_REQUESTOR_H
#define VN_DGNSS_SERVER_BKG_DATA_REQUESTOR_H

#pragma once
#include <arpa/inet.h>

#include "constants.h"
#include "data_struct.h"
#include "time_common_func.h"

struct GnssEphStruct {
  std::vector<satstruct::Ephemeris> GPS_eph;
  std::vector<satstruct::Ephemeris> GAL_eph;
  std::vector<satstruct::Ephemeris> BDS_eph;
  GnssEphStruct() {
    GPS_eph.resize(MAXPRNGPS + 1);
    GAL_eph.resize(MAXPRNGAL + 1);
    BDS_eph.resize(MAXPRNCMP + 1);
  }
};

// Satellite orbit correction parameters for a satellite.
struct SatOrbitPara {
  int prn;
  int IOD{};
  // unit: meter
  std::vector<double> dx_m;
  std::vector<double> dv_m;
  SatOrbitPara() {
    prn = -1;
    dx_m.resize(3);
    dv_m.resize(3);
  }
};

// Satellite orbit correction for one epoch
struct SatOrbitCorrEpoch {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<SatOrbitPara> data_sv;
  SatOrbitCorrEpoch() { datetime.resize(6); }
};

// Satellite clock correction parameters for a satellite.
struct SatClockPara {
  int prn;
  int IOD{};
  // unit: sec
  std::vector<double> dt_corr_s;
  SatClockPara() {
    prn = -1;
    dt_corr_s.resize(3);
  }
};

// Satellite clock correction for one epoch
struct SatClockCorrEpoch {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<SatClockPara> data_sv;
  SatClockCorrEpoch() { datetime.resize(6); }
};

// Multi-GNSS SSR orbit correction for one epoch
struct SsrOrbitCorrEpoch {
  SatOrbitCorrEpoch GPS;
  SatOrbitCorrEpoch GAL;
  SatOrbitCorrEpoch BDS;
  SsrOrbitCorrEpoch() {
    GPS.data_sv.resize(MAXPRNGPS + 1);
    GAL.data_sv.resize(MAXPRNGAL + 1);
    BDS.data_sv.resize(MAXPRNCMP + 1);
  }
};

// Multi-GNSS SSR orbit correction for one epoch
struct SsrClockCorrEpoch {
  SatClockCorrEpoch GPS;
  SatClockCorrEpoch GAL;
  SatClockCorrEpoch BDS;
  SsrClockCorrEpoch() {
    GPS.data_sv.resize(MAXPRNGPS + 1);
    GAL.data_sv.resize(MAXPRNGAL + 1);
    BDS.data_sv.resize(MAXPRNCMP + 1);
  }
};

// Vertical Total Electron Content (VTEC) in ionosphere
struct VTecCorrection {
  //  True when received SSR VTEC
  bool received;

  gtime_t time{};
  std::vector<double> datetime;
  int nDeg{}, nOrd{};

  // meter
  double height_m{};
  std::vector<std::vector<double>> sin_coeffs;
  std::vector<std::vector<double>> cos_coeffs;
  VTecCorrection() { received = false; }
};

// SSR satellite hardware (code/phase) bias element
struct BiasElement {
  // when not available, this value is false
  bool received;

  double value;
  BiasElement() {
    received = false;
    value = 0.0;
  }
};

// Satellite phase bias correction parameters
struct SatPhaseBiasPara {
  int prn;
  double yawdeg{};
  double yawdeg_rate{};
  std::vector<BiasElement> bias_ele;
  SatPhaseBiasPara() {
    prn = -1;
    bias_ele.resize(MAX_CODE_ELEMENTS);
  }
};

// satellite phase bias correction
struct PhaseBiasCorr {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<SatPhaseBiasPara> data;
  PhaseBiasCorr() { datetime.resize(6); }
};

// Multi-GNSS satellite phase bias correction
struct SsrPhaseBiasEpoch {
  PhaseBiasCorr GPS;
  PhaseBiasCorr BDS;
  PhaseBiasCorr GAL;
  SsrPhaseBiasEpoch() {
    GPS.data.resize(MAXPRNGPS + 1);
    GAL.data.resize(MAXPRNGAL + 1);
    BDS.data.resize(MAXPRNCMP + 1);
  }
};

// Satellite code bias correction parameters
struct SatCodeBiasPara {
  int prn;
  std::vector<BiasElement> bias_ele;
  SatCodeBiasPara() {
    prn = -1;
    bias_ele.resize(MAX_CODE_ELEMENTS);
  }
};

// satellite code bias correction
struct CodeBiasCorr {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<SatCodeBiasPara> data;
  CodeBiasCorr() { datetime.resize(6); }
};

// Multi-GNSS satellite code bias correction
struct SsrCodeBiasEpoch {
  CodeBiasCorr GPS;
  CodeBiasCorr BDS;
  CodeBiasCorr GAL;
  SsrCodeBiasEpoch() {
    GPS.data.resize(MAXPRNGPS + 1);
    GAL.data.resize(MAXPRNGAL + 1);
    BDS.data.resize(MAXPRNCMP + 1);
  }
};

class BkgDataRequestor {
 private:
  std::mutex clk_mutex;
  std::mutex obt_mutex;
  std::mutex eph_mutex;
  std::mutex tec_mutex;
  std::mutex cbs_mutex;
  std::mutex pbs_mutex;

  // Log for eph data record
  std::ofstream log_eph;

  // Log for eph data record
  std::ofstream log_ssr;
  const std::string file_path_;

  int ssr_fd{}, eph_fd{};
  pthread_t pid_ssr{}, pid_eph{};
  bool eph_done{}, ssr_done{};
  std::vector<SsrOrbitCorrEpoch> obt_data;
  std::vector<SsrClockCorrEpoch> clk_data;
  std::vector<GnssEphStruct> eph_data;
  VTecCorrection vtec_data;
  SsrCodeBiasEpoch code_bias;
  SsrPhaseBiasEpoch phase_bias;
  std::string eph_log_path[3]{}, ssr_log_path[3]{};

  static void ClearInputStream(std::stringstream &ss, std::string &line);
  int RequestEphData();
  int RequestSsrData();
  static bool BkgSocketClient(int port, const char *ip, int &fd);
  void WriteSsrToLog();
  static void GpsEphParser(std::stringstream &eph_ss,
                           satstruct::Ephemeris &eph_element, gtime_t t_oc);
  static void GalEphParser(std::stringstream &eph_ss,
                           satstruct::Ephemeris &eph_element, gtime_t t_oc);
  static void BdsEphParser(std::stringstream &eph_ss,
                           satstruct::Ephemeris &eph_element, gtime_t t_oc);
  static void SsrVTecParser(VTecCorrection &new_vtec, std::stringstream &ssr_ss,
                            std::string line);
  void RequestEph();
  void RequestSsr();
  static void *RequestEphWrapper(void *arg);
  static void *RequestSsrWrapper(void *arg);

 public:
  bool eph_ready{}, ssr_ready{};
  // No default constructor
  BkgDataRequestor() = delete;
  // constructor
  explicit BkgDataRequestor(std::string log_file_path)
      : file_path_(std::move(log_file_path)) {}
  ~BkgDataRequestor() {
    log_eph.close();
    log_ssr.close();
  }
  // Non-copyable
  BkgDataRequestor(const BkgDataRequestor &) = delete;
  BkgDataRequestor &operator=(const BkgDataRequestor &) = delete;
  // Non-moveable
  BkgDataRequestor(BkgDataRequestor &&) = delete;
  BkgDataRequestor &operator=(BkgDataRequestor &&) = delete;

  std::vector<SsrOrbitCorrEpoch> GetSatOrbitCorrEpochs();
  std::vector<SsrClockCorrEpoch> GetSatClockCorrEpochs();
  std::vector<GnssEphStruct> GetGnssEphDataEpochs();
  VTecCorrection GetSsrVTecCorr();
  SsrCodeBiasEpoch GetSsrCodeBiasCorr();
  SsrPhaseBiasEpoch GetSsrPhaseBiasCorr();

  void StartRequestor();
  void EndRequestor();
};

#endif  // VN_DGNSS_SERVER_BKG_DATA_REQUESTOR_H
