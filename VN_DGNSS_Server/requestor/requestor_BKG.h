
#ifndef WADGNSS_SERVER_REQUESTOR_BKG_H
#define WADGNSS_SERVER_REQUESTOR_BKG_H

#pragma once
#include <arpa/inet.h>
#include "timecmn.h"
#include "SatPosClkComp.h"
#include "requestor_web.h"
struct eph_struct {
  std::vector<SatPosClkComp::Ephemeris> GPS_eph;
  std::vector<SatPosClkComp::Ephemeris> GAL_eph;
  std::vector<SatPosClkComp::Ephemeris> BDS_eph;
  eph_struct() {
    GPS_eph.resize(MAXPRNGPS+1);
    GAL_eph.resize(MAXPRNGAL+1);
    BDS_eph.resize(MAXPRNCMP+1);
  }
};

// Orbit structure for every single line
struct orbit_element {
  int prn;
  int IOD{};
  std::vector<double> dx;
  std::vector<double> dv;
  orbit_element() {
    prn = -1;
    dx.resize(3);
    dv.resize(3);
  }
};
// Orbit data structure for one version
struct orbit_ver {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<orbit_element> data_sv;
  orbit_ver() {
    datetime.resize(6);
  }
};
// Clock structure for every single line
struct clock_element {
  int prn;
  int IOD{};
  std::vector<double> dt_corr;
  clock_element() {
    prn = -1;
    dt_corr.resize(3);
  }
};
// Clock data structure
struct clock_ver {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<clock_element> data_sv;
  clock_ver() {
    datetime.resize(6);
  }
};
// Generate block for different system to aviod data conflict from BKG
struct ssr_orbit {
  orbit_ver GPS;
  orbit_ver GAL;
  orbit_ver BDS;
  ssr_orbit() {
    GPS.data_sv.resize(MAXPRNGPS+1);
    GAL.data_sv.resize(MAXPRNGAL+1);
    BDS.data_sv.resize(MAXPRNCMP+1);
  }
};
struct ssr_clock {
  clock_ver GPS;
  clock_ver GAL;
  clock_ver BDS;
  ssr_clock() {
    GPS.data_sv.resize(MAXPRNGPS+1);
    GAL.data_sv.resize(MAXPRNGAL+1);
    BDS.data_sv.resize(MAXPRNCMP+1);
  }
};

#define MAXVTEC    15
#define EPH_VERMAX 10
struct vtec_t{        /* Vertical Total Electron Content (VTEC) in ionosphere */
  bool check; //  True when received SSR VTEC
  gtime_t time{};
  std::vector<double> datetime;
  int nDeg{}, nOrd{};
  double height{}; // meter
  std::vector<std::vector<double>> sinCoeffs;
  std::vector<std::vector<double>> cosCoeffs;
  vtec_t() {
    check = false;
  }
};

struct single_bias{
  bool check; // when not available, this value is false
  double value;
  single_bias() {
    check = false;
    value = 0.0;
  }
};
// Define phase bias struct
struct pbias_sys {
  int prn;
  double yawdeg{};
  double yawdeg_rate{};
  std::vector<single_bias> bias_ele;
  pbias_sys() {
    prn = -1;
    bias_ele.resize(MAX_CODE_ELE);
  }
};
struct pbias_ver {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<pbias_sys> data;
  pbias_ver() {
    datetime.resize(6);
  }
};
struct pbias_ssr {
  pbias_ver GPS;
  pbias_ver BDS;
  pbias_ver GAL;
  pbias_ssr() {
    GPS.data.resize(MAXPRNGPS+1);
    GAL.data.resize(MAXPRNGAL+1);
    BDS.data.resize(MAXPRNCMP+1);
  }
};
// Define code bias struct
struct cbias_sys {
  int prn;
  std::vector<single_bias> bias_ele;
  cbias_sys() {
    prn = -1;
    bias_ele.resize(MAX_CODE_ELE);
  }
};
struct cbias_ver {
  std::vector<double> datetime;
  gtime_t time{};
  std::vector<cbias_sys> data;
  cbias_ver() {
    datetime.resize(6);
  }
};
struct cbias_ssr {
  cbias_ver GPS;
  cbias_ver BDS;
  cbias_ver GAL;
  cbias_ssr() {
    GPS.data.resize(MAXPRNGPS+1);
    GAL.data.resize(MAXPRNGAL+1);
    BDS.data.resize(MAXPRNCMP+1);
  }
};

class requestor_BKG {
private:
  std::mutex clk_mutex;
  std::mutex obt_mutex;
  std::mutex eph_mutex;
  std::mutex tec_mutex;
  std::mutex cbs_mutex;
  std::mutex pbs_mutex;
  std::ofstream log_eph; // Log for eph data record
  std::ofstream log_ssr; // Log for eph data record
  const std::string FILE_PATH;

  int ssr_fd{}, eph_fd{};
  pthread_t pid_ssr{}, pid_eph{};
  bool eph_done{}, ssr_done{};
  std::vector<ssr_orbit> obt_data;
  std::vector<ssr_clock> clk_data;
  std::vector<eph_struct> eph_data;
  vtec_t vtec_data;
  cbias_ssr code_bias;
  pbias_ssr phase_bias;
  std::string eph_logpath[3]{}, ssr_logpath[3]{};

  static void clear_input_stream(std::stringstream &ss, std::string &line);
  int request_eph_data();
  int request_ssr_data();
  static bool bkg_socket_client(int port, const char *IP, int &fd);
  void write_ssr_to_log();
  static void GPS_eph_parser(std::stringstream &eph_ss,
                             SatPosClkComp::Ephemeris & eph_element,
                             gtime_t t_oc);
  static void GAL_eph_parser(std::stringstream &eph_ss,
                             SatPosClkComp::Ephemeris & eph_element,
                             gtime_t t_oc);
  static void BDS_eph_parser(std::stringstream &eph_ss,
                             SatPosClkComp::Ephemeris & eph_element,
                             gtime_t t_oc);
  static void parse_vtec(vtec_t &new_vtec, std::stringstream &ssr_ss,
                           std::string line);
  void request_eph();
  void request_ssr();
  static void *request_eph_wrapper(void *arg);
  static void *request_ssr_wrapper(void *arg);

public:
  bool eph_ready{}, ssr_ready{};
  // No default constructor
  requestor_BKG() = delete;
  // constructor
  explicit requestor_BKG(std::string log_FILE_PATH)
  : FILE_PATH(std::move(log_FILE_PATH)){}
  ~requestor_BKG() { log_eph.close(); log_ssr.close();}
  // Non-copyable
  requestor_BKG(const requestor_BKG &) = delete;
  requestor_BKG &operator=(const requestor_BKG &) = delete;
  // Non-moveable
  requestor_BKG(requestor_BKG &&) = delete;
  requestor_BKG &operator=(requestor_BKG &&) = delete;

  std::vector<ssr_orbit> get_orbit_data();
  std::vector<ssr_clock> get_clock_data();
  std::vector<eph_struct> get_eph_data();
  vtec_t get_vtec_data();
  cbias_ssr get_code_bias();
  pbias_ssr get_phase_bias();

  void start_request();
  void end_request();
};

#endif // WADGNSS_SERVER_REQUESTOR_BKG_H
