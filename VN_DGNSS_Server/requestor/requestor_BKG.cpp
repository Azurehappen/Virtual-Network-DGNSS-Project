
#include "requestor_BKG.h"

// IP for BKG data output
static constexpr const char *local_ip = "127.0.0.1";
// IP port for Eph data
static constexpr int EPH_port = 3536;
// IP port for SSR data
static constexpr int SSR_port = 6699;
// the request SSR&EPH interval, 0.1s
static constexpr int reqst_interval = 100000;
// log period of IGS corr data in seconds
static constexpr int SSRLOG_PERIOD = 1800;
// log file update period in seconds (1 day)
static constexpr int FILE_PERIOD = 86400;
// check IP port period in microseconds (2 s)
static constexpr int CHECK_PERIOD = 2 * 1000000;

/* adjust time considering week handover -------------------------------------*/
static gtime_t adjweek(gtime_t t, gtime_t t0) {
  double tt = timediff(t, t0);
  if (tt < -302400.0) return timeadd(t, 604800.0);
  if (tt > 302400.0) return timeadd(t, -604800.0);
  return t;
}

// Clear stringstream and input line string
void requestor_BKG::clear_input_stream(std::stringstream &ss,
                                       std::string &line) {
  ss.clear();
  ss.str("");
  ss << line;
}

static clock_element parse_clk_elem(const std::string &line) {
  std::stringstream ss(line);
  clock_element new_element;
  ss >> new_element.prn >> new_element.IOD;
  for (auto &corr : new_element.dt_corr) {
    ss >> corr;
  }
  return new_element;
}

static orbit_element parse_obt_elem(const std::string &line) {
  std::stringstream ss(line);
  orbit_element new_element;
  ss >> new_element.prn >> new_element.IOD;
  for (auto &dx : new_element.dx) {
    ss >> dx;
  }
  for (auto &dv : new_element.dv) {
    ss >> dv;
  }
  return new_element;
}
void requestor_BKG::parse_vtec(vtec_t &new_vtec, std::stringstream &ssr_ss,
                               std::string line) {
  std::stringstream line_ss(line);
  int dummy;
  line_ss >> dummy >> new_vtec.nDeg >> new_vtec.nOrd >> new_vtec.height;
  new_vtec.sinCoeffs.resize(new_vtec.nDeg + 1);
  new_vtec.cosCoeffs.resize(new_vtec.nDeg + 1);
  for (int nn = 0; nn <= new_vtec.nDeg; nn++) {
    new_vtec.sinCoeffs[nn].resize(new_vtec.nOrd + 1);
    new_vtec.cosCoeffs[nn].resize(new_vtec.nOrd + 1);
  }
  for (int iDeg = 0; iDeg <= new_vtec.nDeg; iDeg++) {
    getline(ssr_ss, line);
    clear_input_stream(line_ss, line);
    for (int iOrd = 0; iOrd <= new_vtec.nOrd; iOrd++) {
      line_ss >> new_vtec.cosCoeffs[iDeg][iOrd];
    }
  }
  for (int iDeg = 0; iDeg <= new_vtec.nDeg; iDeg++) {
    getline(ssr_ss, line);
    clear_input_stream(line_ss, line);
    for (int iOrd = 0; iOrd <= new_vtec.nOrd; iOrd++) {
      line_ss >> new_vtec.sinCoeffs[iDeg][iOrd];
    }
  }
}

static void parse_ssr_cbias(cbias_sys &new_element, const std::string &line,
                            const std::string &sys) {
  // Due to the limitation in phase_bias
  // only parse 1C and 2W for GPS, 1x and 7x for GAL, 2I and 7I for BDS
  std::stringstream line_ss(line);
  int prn, num;
  line_ss >> prn >> num;
  if (prn > 0) {
    new_element.prn = prn;
  }
  if (num > 0) {
    for (int i = 0; i < num; i++) {
      std::string code;
      double value;
      line_ss >> code >> value;
      if (sys == "G") {
        if (code == "1C") {
          new_element.bias_ele[code_GPS_C1C].check = true;
          new_element.bias_ele[code_GPS_C1C].value = value;
        } else if (code == "2W") {
          new_element.bias_ele[code_GPS_C2W].check = true;
          new_element.bias_ele[code_GPS_C2W].value = value;
        }
      } else if (sys == "E") {
        if (code == "1X") {
          new_element.bias_ele[code_GAL_C1X].check = true;
          new_element.bias_ele[code_GAL_C1X].value = value;
        } else if (code == "7X") {
          new_element.bias_ele[code_GAL_C7X].check = true;
          new_element.bias_ele[code_GAL_C7X].value = value;
        }
      } else if (sys == "C") {
        if (code == "2I") {
          new_element.bias_ele[code_BDS_C2I].check = true;
          new_element.bias_ele[code_BDS_C2I].value = value;
        } else if (code == "7I" || code == "7Z") {
          new_element.bias_ele[code_BDS_C7].check = true;
          new_element.bias_ele[code_BDS_C7].value = value;
        }
      }
    }
  }
}
static void parse_ssr_pbias(pbias_sys &new_element, const std::string &line,
                            const std::string &sys) {
  // Due to the limitation in phase_bias
  // only parse 1C and 2W for GPS, 1x and 7x for GAL, 2I and 7I for BDS
  std::stringstream line_ss(line);
  int prn, num;
  double yaw, yawrate;
  line_ss >> prn >> yaw >> yawrate >> num;
  if (prn > 0) {
    new_element.prn = prn;
    new_element.yawdeg = yaw;
    new_element.yawdeg_rate = yawrate;
  }
  if (num > 0) {
    for (int i = 0; i < num; i++) {
      std::string code;
      double value, ind1, ind2, ind3;
      line_ss >> code >> value >> ind1 >> ind2 >> ind3;
      if (sys == "G") {
        if (code == "1C") {
          new_element.bias_ele[code_GPS_C1C].check = true;
          new_element.bias_ele[code_GPS_C1C].value = value;
        } else if (code == "2W") {
          new_element.bias_ele[code_GPS_C2W].check = true;
          new_element.bias_ele[code_GPS_C2W].value = value;
        }
      } else if (sys == "E") {
        if (code == "1X") {
          new_element.bias_ele[code_GAL_C1X].check = true;
          new_element.bias_ele[code_GAL_C1X].value = value;
        } else if (code == "7X") {
          new_element.bias_ele[code_GAL_C7X].check = true;
          new_element.bias_ele[code_GAL_C7X].value = value;
        }
      } else if (sys == "C") {
        if (code == "2I") {
          new_element.bias_ele[code_BDS_C2I].check = true;
          new_element.bias_ele[code_BDS_C2I].value = value;
        } else if (code == "7I" || code == "7Z") {
          new_element.bias_ele[code_BDS_C7].check = true;
          new_element.bias_ele[code_BDS_C7].value = value;
        }
      }
    }
  }
}
// Request IGS data, return msg_num on success
int requestor_BKG::request_ssr_data() {
  bool recv_clk = false, recv_obt = false;
  bool recv_cbs = false, recv_pbs = false;
  char IGS_buf[150000] = {0};
  int msg_num = 0;  // message received check: 0 none, 1 received
  int IGS_ret = recv(ssr_fd, IGS_buf, sizeof(IGS_buf), MSG_DONTWAIT);
  if (IGS_ret == -1) {
    if (!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
      log_ssr << get_time() << "IGS port unexpected error: " << strerror(errno)
              << std::endl;
      return -1;
    } else {
      return 0;
    }
  } else if (IGS_ret == 0) {
    log_ssr << get_time() << "BKG has closed IGS port connection." << std::endl;
    return -1;
  } else if (IGS_ret > 0) {
    // Initialize data struct for SSR
    clock_ver new_clk_gps, new_clk_gal, new_clk_bds;
    new_clk_gps.data_sv.resize(MAXPRNGPS + 1);
    new_clk_gal.data_sv.resize(MAXPRNGAL + 1);
    new_clk_bds.data_sv.resize(MAXPRNCMP + 1);
    orbit_ver new_obt_gps, new_obt_gal, new_obt_bds;
    new_obt_gps.data_sv.resize(MAXPRNGPS + 1);
    new_obt_gal.data_sv.resize(MAXPRNGAL + 1);
    new_obt_bds.data_sv.resize(MAXPRNCMP + 1);
    cbias_ver new_cbs_gps, new_cbs_gal, new_cbs_bds;
    new_cbs_gps.data.resize(MAXPRNGPS + 1);
    new_cbs_gal.data.resize(MAXPRNGAL + 1);
    new_cbs_bds.data.resize(MAXPRNCMP + 1);
    pbias_ver new_pbs_gps, new_pbs_gal, new_pbs_bds;
    new_pbs_gps.data.resize(MAXPRNGPS + 1);
    new_pbs_gal.data.resize(MAXPRNGAL + 1);
    new_pbs_bds.data.resize(MAXPRNCMP + 1);
    std::vector<double> datetime(6, 3);
    gtime_t ssr_time;
    std::stringstream ssr_ss(IGS_buf), line_ss;
    std::string line, symbol;
    std::string type{};
    // Check which systems in the data block
    bool get_gps_clk = false, get_gal_clk = false, get_bds_clk = false;
    bool get_gps_obt = false, get_gal_obt = false, get_bds_obt = false;
    bool get_gps_cbs = false, get_gal_cbs = false, get_bds_cbs = false;
    bool get_gps_pbs = false, get_gal_pbs = false, get_bds_pbs = false;
    while (!ssr_ss.eof()) {
      getline(ssr_ss, line);
      if (line[0] == '>') {
        clear_input_stream(line_ss, line);
        line_ss >> symbol >> type;
        if (type == "CLOCK" || type == "ORBIT" || type == "VTEC" ||
            type == "CODE_BIAS" || type == "PHASE_BIAS") {
          line_ss >> datetime[0] >> datetime[1] >> datetime[2] >> datetime[3] >>
              datetime[4] >> datetime[5];
          ssr_time = epoch2time(datetime);
          // go to next line
          continue;
        } else {
          continue;
        }
      }
      std::string gpsdef("G"), galdef("E"), bdsdef("C");
      // Read data
      if (type == "CLOCK") {
        std::string sys = line.substr(0, 1);
        if (sys == gpsdef || sys == galdef || sys == bdsdef) {
          recv_clk = true;
          // ignore the first character, will be used later
          line = line.substr(1, line.size() - 1);
          clock_element new_element = parse_clk_elem(line);
          if (sys == gpsdef && new_element.prn != -1) {
            if (!get_gps_clk) {
              new_clk_gps.datetime = datetime;
              new_clk_gps.time = ssr_time;
              get_gps_clk = true;
            }
            new_clk_gps.data_sv[new_element.prn] = new_element;
          } else if (sys == galdef && new_element.prn != -1) {
            if (!get_gal_clk) {
              new_clk_gal.datetime = datetime;
              new_clk_gal.time = ssr_time;
              get_gal_clk = true;
            }
            new_clk_gal.data_sv[new_element.prn] = new_element;
          } else if (sys == bdsdef && new_element.prn != -1) {
            if (!get_bds_clk) {
              new_clk_bds.datetime = datetime;
              new_clk_bds.time = ssr_time;
              get_bds_clk = true;
            }
            if (new_element.prn != -1) {
              new_clk_bds.data_sv[new_element.prn] = new_element;
            }
          }
        }
      } else if (type == "ORBIT") {
        std::string sys = line.substr(0, 1);
        if (sys == gpsdef || sys == galdef || sys == bdsdef) {
          recv_obt = true;
          line = line.substr(1, line.size() - 1);
          orbit_element new_element = parse_obt_elem(line);
          if (sys == gpsdef && new_element.prn != -1) {
            if (!get_gps_obt) {
              new_obt_gps.datetime = datetime;
              new_obt_gps.time = ssr_time;
              get_gps_obt = true;
            }
            new_obt_gps.data_sv[new_element.prn] = new_element;
          } else if (sys == galdef && new_element.prn != -1) {
            if (!get_gal_obt) {
              new_obt_gal.datetime = datetime;
              new_obt_gal.time = ssr_time;
              get_gal_obt = true;
            }
            new_obt_gal.data_sv[new_element.prn] = new_element;
          } else if (sys == bdsdef && new_element.prn != -1) {
            if (!get_bds_obt) {
              new_obt_bds.datetime = datetime;
              new_obt_bds.time = ssr_time;
              get_bds_obt = true;
            }
            if (new_element.prn != -1) {
              new_obt_bds.data_sv[new_element.prn] = new_element;
            }
          }
        }
      } else if (type == "VTEC") {
        vtec_t new_vtec;
        new_vtec.check = true;
        new_vtec.time = ssr_time;
        new_vtec.datetime = datetime;
        parse_vtec(new_vtec, ssr_ss, line);
        msg_num = 1;
        std::lock_guard<std::mutex> lock(tec_mutex);
        vtec_data = new_vtec;
        type = {};
      } else if (type == "CODE_BIAS") {
        std::string sys = line.substr(0, 1);
        if (sys == gpsdef || sys == galdef || sys == bdsdef) {
          recv_cbs = true;
          line = line.substr(1, line.size() - 1);
          cbias_sys new_element;
          if (sys == gpsdef) {
            if (!get_gps_cbs) {
              new_cbs_gps.datetime = datetime;
              new_cbs_gps.time = ssr_time;
              get_gps_cbs = true;
            }
            parse_ssr_cbias(new_element, line, sys);
            if (new_element.prn != -1) {
              new_cbs_gps.data[new_element.prn] = new_element;
            }
          } else if (sys == galdef) {
            if (!get_gal_cbs) {
              new_cbs_gal.datetime = datetime;
              new_cbs_gal.time = ssr_time;
              get_gal_cbs = true;
            }
            parse_ssr_cbias(new_element, line, sys);
            if (new_element.prn != -1) {
              new_cbs_gal.data[new_element.prn] = new_element;
            }
          } else if (sys == bdsdef) {
            if (!get_bds_cbs) {
              new_cbs_bds.datetime = datetime;
              new_cbs_bds.time = ssr_time;
              get_bds_cbs = true;
            }
            parse_ssr_cbias(new_element, line, sys);
            if (new_element.prn != -1) {
              new_cbs_bds.data[new_element.prn] = new_element;
            }
          }
        }
      } else if (type == "PHASE_BIAS") {
        std::string sys = line.substr(0, 1);
        if (sys == gpsdef || sys == galdef || sys == bdsdef) {
          recv_pbs = true;
          line = line.substr(1, line.size() - 1);
          pbias_sys new_element;
          if (sys == gpsdef) {
            if (!get_gps_pbs) {
              new_pbs_gps.datetime = datetime;
              new_pbs_gps.time = ssr_time;
              get_gps_pbs = true;
            }
            parse_ssr_pbias(new_element, line, sys);
            if (new_element.prn != -1 && new_element.prn <= MAXPRNGPS) {
              new_pbs_gps.data[new_element.prn] = new_element;
            }
          } else if (sys == galdef) {
            if (!get_gal_pbs) {
              new_pbs_gal.datetime = datetime;
              new_pbs_gal.time = ssr_time;
              get_gal_pbs = true;
            }
            parse_ssr_pbias(new_element, line, sys);
            if (new_element.prn != -1 && new_element.prn <= MAXPRNGAL) {
              new_pbs_gal.data[new_element.prn] = new_element;
            }
          } else if (sys == bdsdef) {
            if (!get_bds_pbs) {
              new_pbs_bds.datetime = datetime;
              new_pbs_bds.time = ssr_time;
              get_bds_pbs = true;
            }
            parse_ssr_pbias(new_element, line, sys);
            if (new_element.prn != -1 && new_element.prn <= MAXPRNCMP) {
              new_pbs_bds.data[new_element.prn] = new_element;
            }
          }
        }
      }
    }
    // update clock data for different version
    if (recv_clk) {
      msg_num = 1;
      std::lock_guard<std::mutex> lock(clk_mutex);
      if (get_gps_clk) {
        clk_data[2].GPS = clk_data[1].GPS;
        clk_data[1].GPS = clk_data[0].GPS;
        clk_data[0].GPS = new_clk_gps;
      }
      if (get_gal_clk) {
        clk_data[2].GAL = clk_data[1].GAL;
        clk_data[1].GAL = clk_data[0].GAL;
        clk_data[0].GAL = new_clk_gal;
      }
      if (get_bds_clk) {
        clk_data[2].BDS = clk_data[1].BDS;
        clk_data[1].BDS = clk_data[0].BDS;
        clk_data[0].BDS = new_clk_bds;
      }
    }
    // update orbit data for different version
    if (recv_obt) {
      msg_num = 1;
      std::lock_guard<std::mutex> lock(obt_mutex);
      if (get_gps_obt) {
        obt_data[2].GPS = obt_data[1].GPS;
        obt_data[1].GPS = obt_data[0].GPS;
        obt_data[0].GPS = new_obt_gps;
      }
      if (get_gal_obt) {
        obt_data[2].GAL = obt_data[1].GAL;
        obt_data[1].GAL = obt_data[0].GAL;
        obt_data[0].GAL = new_obt_gal;
      }
      if (get_bds_obt) {
        obt_data[2].BDS = obt_data[1].BDS;
        obt_data[1].BDS = obt_data[0].BDS;
        obt_data[0].BDS = new_obt_bds;
      }
    }
    // update code bias data
    if (recv_cbs) {
      msg_num = 1;
      std::lock_guard<std::mutex> lock(cbs_mutex);
      if (get_gps_cbs) {
        code_bias.GPS = new_cbs_gps;
      }
      if (get_gal_cbs) {
        code_bias.GAL = new_cbs_gal;
      }
      if (get_bds_cbs) {
        code_bias.BDS = new_cbs_bds;
      }
    }
    // update phase bias data
    if (recv_pbs) {
      msg_num = 1;
      std::lock_guard<std::mutex> lock(pbs_mutex);
      if (get_gps_pbs) {
        phase_bias.GPS = new_pbs_gps;
      }
      if (get_gal_pbs) {
        phase_bias.GAL = new_pbs_gal;
      }
      if (get_bds_pbs) {
        phase_bias.BDS = new_pbs_bds;
      }
    }
  }
  return msg_num;
}

void requestor_BKG::GPS_eph_parser(std::stringstream &eph_ss,
                                   SatStruct::Ephemeris &eph_element,
                                   gtime_t t_oc) {
  std::stringstream ss;
  std::string line;
  // read line 2
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double IODE;
  ss >> IODE >> eph_element.C_rs >> eph_element.Delta_n >> eph_element.M_0;
  eph_element.IODE = static_cast<size_t>(IODE);
  // read line 3
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.C_uc >> eph_element.e >> eph_element.C_us >>
      eph_element.sqrtA;
  // read line 4
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.toes >> eph_element.C_ic >> eph_element.Omega_0 >>
      eph_element.C_is;
  // read line 5
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.i_0 >> eph_element.C_rc >> eph_element.omega >>
      eph_element.OmegaDot;
  // read line 6
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double Codes, L2, weekNo;
  ss >> eph_element.IDOT >> Codes >> weekNo >> L2;
  eph_element.weekNo = static_cast<size_t>(weekNo);
  eph_element.t_oe =
      adjweek(gpst2time(eph_element.weekNo, eph_element.toes), t_oc);
  // read line 7
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double svHealth, IODC;
  ss >> eph_element.svAcc >> svHealth >> eph_element.T_GD >> IODC;
  eph_element.IODC = static_cast<size_t>(IODC);
  eph_element.svH = static_cast<int>(svHealth);
  // read line 8
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double transmission, fit;
  ss >> transmission >> fit;
}

void requestor_BKG::GAL_eph_parser(std::stringstream &eph_ss,
                                   SatStruct::Ephemeris &eph_element,
                                   gtime_t t_oc) {
  std::stringstream ss;
  std::string line;
  // read line 2
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double IODE;
  ss >> IODE >> eph_element.C_rs >> eph_element.Delta_n >> eph_element.M_0;
  eph_element.IODE = static_cast<size_t>(IODE);
  // read line 3
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.C_uc >> eph_element.e >> eph_element.C_us >>
      eph_element.sqrtA;
  // read line 4
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.toes >> eph_element.C_ic >> eph_element.Omega_0 >>
      eph_element.C_is;
  // read line 5
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.i_0 >> eph_element.C_rc >> eph_element.omega >>
      eph_element.OmegaDot;
  // read line 6
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double Codes, weekNo;
  ss >> eph_element.IDOT >> Codes >> weekNo;
  eph_element.weekNo = static_cast<size_t>(weekNo);
  // In RINEX, GAL week aligned to GPS week.
  // GAL seconds of week equal to GPS seconds of week
  eph_element.t_oe =
      adjweek(gpst2time(eph_element.weekNo, eph_element.toes), t_oc);
  eph_element.data_src = static_cast<int>(Codes);
  // read line 7
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double svHealth;
  ss >> eph_element.svAcc >> svHealth >> eph_element.T_GD >> eph_element.T_GD2;
  eph_element.svH = static_cast<int>(svHealth);
  // read line 8
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double transmission;
  ss >> transmission;
}

void requestor_BKG::BDS_eph_parser(std::stringstream &eph_ss,
                                   SatStruct::Ephemeris &eph_element,
                                   gtime_t t_oc) {
  std::stringstream ss;
  std::string line;
  // read line 2
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double AODE;
  ss >> AODE >> eph_element.C_rs >> eph_element.Delta_n >> eph_element.M_0;
  // read line 3
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.C_uc >> eph_element.e >> eph_element.C_us >>
      eph_element.sqrtA;
  // read line 4
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.toes >> eph_element.C_ic >> eph_element.Omega_0 >>
      eph_element.C_is;
  eph_element.IODE = ((int)(eph_element.toes / 720)) % 240;
  // read line 5
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  ss >> eph_element.i_0 >> eph_element.C_rc >> eph_element.omega >>
      eph_element.OmegaDot;
  // read line 6
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double Spare1, Spare2, weekNo;
  ss >> eph_element.IDOT >> Spare1 >> weekNo >> Spare2;
  eph_element.weekNo = static_cast<size_t>(weekNo);
  gtime_t t_oe = bdt2gpst(bdt2time(eph_element.weekNo, eph_element.toes));
  eph_element.t_oe = adjweek(t_oe, t_oc);
  // read line 7
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double svHealth;
  ss >> eph_element.svAcc >> svHealth >> eph_element.T_GD >> eph_element.T_GD2;
  eph_element.svH = static_cast<int>(svHealth);
  // read line 8
  getline(eph_ss, line);
  clear_input_stream(ss, line);
  double transmission, AODC;
  ss >> transmission >> AODC;
  eph_element.IODC = static_cast<size_t>(AODC);
}

// Request ephemeris data, return true on success
// return - 1 for error, 0 for no sv update, >0 for no. of updated sv
int requestor_BKG::request_eph_data() {
  char eph_buf[100000] = {0};
  int num_sv = 0;
  int eph_ret = recv(eph_fd, eph_buf, sizeof(eph_buf), MSG_DONTWAIT);
  if (eph_ret == -1) {
    if (!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
      log_eph << get_time() << "EPH port unexpected error: " << strerror(errno)
              << std::endl;
      return -1;
    } else {
      return -2;
    }
  } else if (eph_ret == 0) {
    log_eph << get_time() << "BKG has closed EPH port connection." << std::endl;
    return -1;
  } else if (eph_ret > 0) {
    std::stringstream eph_ss(eph_buf), ss;
    std::string line, type{}, sv_rcrd{};
    while (!eph_ss.eof()) {
      getline(eph_ss, line);
      if (line[0] == 'G' || line[0] == 'E' || line[0] == 'C') {
        // read line 1
        SatStruct::Ephemeris eph_element{};
        clear_input_stream(ss, line);
        char system;
        double teph[6];
        ss >> system >> eph_element.prn >> teph[0] >> teph[1] >> teph[2] >>
            teph[3] >> teph[4] >> teph[5] >> eph_element.a_f0 >>
            eph_element.a_f1 >> eph_element.a_f2;
        if (teph[0] < 2000) teph[0] += 2000;
        eph_element.t_oc = epoch2time(teph);
        int check = 0;  // If this eph data exist, :while" continue
        int log_iod_new, log_iod_old;
        if (line[0] == 'G') {
          GPS_eph_parser(eph_ss, eph_element, eph_element.t_oc);
          log_iod_new = eph_element.IODE;
          log_iod_old = eph_data[0].GPS_eph[eph_element.prn].IODE;
          for (auto &i : eph_data) {
            if (eph_element.IODE == i.GPS_eph[eph_element.prn].IODE &&
                i.GPS_eph[eph_element.prn].prn != -1) {
              check = 1;
              continue;
            }
          }
        } else if (line[0] == 'E') {
          GAL_eph_parser(eph_ss, eph_element, eph_element.t_oc);
          // only reserve I/NAV massage (Date source: 517 for I/NAV, 258 for
          // F/NAV)
          if (eph_element.data_src != 517) {
            check = 1;
          }
          log_iod_new = eph_element.IODE;
          log_iod_old = eph_data[0].GAL_eph[eph_element.prn].IODE;
          for (auto &i : eph_data) {
            if (eph_element.IODE == i.GAL_eph[eph_element.prn].IODE &&
                i.GAL_eph[eph_element.prn].prn != -1) {
              check = 1;
              continue;
            }
          }
        } else if (line[0] == 'C') {
          eph_element.t_oc = bdt2gpst(eph_element.t_oc);
          BDS_eph_parser(eph_ss, eph_element, eph_element.t_oc);
          log_iod_new = eph_element.IODE;
          log_iod_old = eph_data[0].BDS_eph[eph_element.prn].IODE;
          for (auto &i : eph_data) {
            if (eph_element.IODE == i.BDS_eph[eph_element.prn].IODE &&
                i.BDS_eph[eph_element.prn].prn != -1) {
              check = 1;
              continue;
            }
          }
        }
        if (check == 0) {  // When check = 1, nothing update
          num_sv++;
          log_eph << line[0] << eph_element.prn << " IODE " << log_iod_old
                  << " updated to " << log_iod_new << std::endl;
          sv_rcrd.append(line[0] + std::to_string(eph_element.prn) + " ");
          if (line[0] == 'G') {
            std::lock_guard<std::mutex> lock(eph_mutex);
            for (int ver = EPH_VERMAX - 1; ver > 0; ver--) {
              eph_data[ver].GPS_eph[eph_element.prn] =
                  eph_data[ver - 1].GPS_eph[eph_element.prn];
            }
            eph_data[0].GPS_eph[eph_element.prn] = eph_element;
          } else if (line[0] == 'E') {
            std::lock_guard<std::mutex> lock(eph_mutex);
            for (int ver = EPH_VERMAX - 1; ver > 0; ver--) {
              eph_data[ver].GAL_eph[eph_element.prn] =
                  eph_data[ver - 1].GAL_eph[eph_element.prn];
            }
            eph_data[0].GAL_eph[eph_element.prn] = eph_element;
          } else if (line[0] == 'C') {
            std::lock_guard<std::mutex> lock(eph_mutex);
            for (int ver = EPH_VERMAX - 1; ver > 0; ver--) {
              eph_data[ver].BDS_eph[eph_element.prn] =
                  eph_data[ver - 1].BDS_eph[eph_element.prn];
            }
            eph_data[0].BDS_eph[eph_element.prn] = eph_element;
          }
        }
      }
    }
    if (num_sv > 0) {
      log_eph << get_time() << "recv sv prn: " << sv_rcrd << "data"
              << std::endl;
    }
    return num_sv;
  }
  return num_sv;
}

// Define the fd for EPH or IGS data stream
bool requestor_BKG::bkg_socket_client(int port, const char *IP, int &fd) {
  // create a socket
  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    std::cerr << "socket start error: " << strerror(errno) << std::endl;
    return false;
  }
  // connect local ip
  struct sockaddr_in server_addr {};
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, IP, &server_addr.sin_addr.s_addr);
  int check = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (check == -1) {
    std::cerr << "connection start error: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }
  //  struct timeval timeout_recv = {0, 200000};  // 0.2s
  //  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_recv,
  //             sizeof(timeout_recv));
  return true;
}

// Output IGS data to log file
void requestor_BKG::write_ssr_to_log() {
  log_ssr << get_time() << "Current clock data" << '\n';
  clock_ver clk_gps = clk_data[0].GPS;
  log_ssr << "GPS: " << clk_gps.datetime[0] << "-" << clk_gps.datetime[1] << "-"
          << clk_gps.datetime[2] << " " << clk_gps.datetime[3] << ":"
          << clk_gps.datetime[4] << ":" << clk_gps.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNGPS + 1; i++) {
    if (clk_gps.data_sv[i].prn != -1) {
      log_ssr << "G-" << clk_gps.data_sv[i].prn
              << " IOD: " << clk_gps.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << clk_gps.data_sv[i].dt_corr[k];
      }
      log_ssr << '\n';
    }
  }
  clock_ver clk_gal = clk_data[0].GAL;
  log_ssr << "GAL: " << clk_gal.datetime[0] << "-" << clk_gal.datetime[1] << "-"
          << clk_gal.datetime[2] << " " << clk_gal.datetime[3] << ":"
          << clk_gal.datetime[4] << ":" << clk_gal.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNGAL + 1; i++) {
    if (clk_gal.data_sv[i].prn != -1) {
      log_ssr << "E-" << clk_gal.data_sv[i].prn
              << " IOD: " << clk_gal.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << clk_gal.data_sv[i].dt_corr[k];
      }
      log_ssr << '\n';
    }
  }
  clock_ver clk_bds = clk_data[0].BDS;
  log_ssr << "BDS: " << clk_bds.datetime[0] << "-" << clk_bds.datetime[1] << "-"
          << clk_bds.datetime[2] << " " << clk_bds.datetime[3] << ":"
          << clk_bds.datetime[4] << ":" << clk_bds.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNCMP + 1; i++) {
    if (clk_bds.data_sv[i].prn != -1) {
      log_ssr << "C-" << clk_bds.data_sv[i].prn
              << " IOD: " << clk_bds.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << clk_bds.data_sv[i].dt_corr[k];
      }
      log_ssr << '\n';
    }
  }

  log_ssr << get_time() << "Current orbit data" << '\n';
  orbit_ver obt_gps = obt_data[0].GPS;
  log_ssr << "GPS: " << obt_gps.datetime[0] << "-" << obt_gps.datetime[1] << "-"
          << obt_gps.datetime[2] << " " << obt_gps.datetime[3] << ":"
          << obt_gps.datetime[4] << ":" << obt_gps.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNGPS + 1; i++) {
    if (obt_gps.data_sv[i].prn != -1) {
      log_ssr << "G-" << obt_gps.data_sv[i].prn
              << " IOD: " << obt_gps.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_gps.data_sv[i].dx[k];
      }
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_gps.data_sv[i].dv[k];
      }
      log_ssr << '\n';
    }
  }
  orbit_ver obt_gal = obt_data[0].GAL;
  log_ssr << "GAL: " << obt_gal.datetime[0] << "-" << obt_gal.datetime[1] << "-"
          << obt_gal.datetime[2] << " " << obt_gal.datetime[3] << ":"
          << obt_gal.datetime[4] << ":" << obt_gal.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNGAL + 1; i++) {
    if (obt_gal.data_sv[i].prn != -1) {
      log_ssr << "E-" << obt_gal.data_sv[i].prn
              << " IOD: " << obt_gal.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_gal.data_sv[i].dx[k];
      }
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_gal.data_sv[i].dv[k];
      }
      log_ssr << '\n';
    }
  }
  orbit_ver obt_bds = obt_data[0].BDS;
  log_ssr << "BDS: " << obt_bds.datetime[0] << "-" << obt_bds.datetime[1] << "-"
          << obt_bds.datetime[2] << " " << obt_bds.datetime[3] << ":"
          << obt_bds.datetime[4] << ":" << obt_bds.datetime[5] << '\n';
  for (int i = 1; i < MAXPRNCMP + 1; i++) {
    if (obt_bds.data_sv[i].prn != -1) {
      log_ssr << "C-" << obt_bds.data_sv[i].prn
              << " IOD: " << obt_bds.data_sv[i].IOD;
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_bds.data_sv[i].dx[k];
      }
      for (int k = 0; k < 3; k++) {
        log_ssr << " " << obt_bds.data_sv[i].dv[k];
      }
      log_ssr << '\n';
    }
  }
  log_ssr << std::endl;
}

void requestor_BKG::request_eph() {
  if (!bkg_socket_client(EPH_port, local_ip, eph_fd)) {
    std::cerr << "Please check if the eph data port enable" << std::endl;
    pthread_exit(nullptr);
  }
  // keep requesting until the latest version
  while (true) {
    int eph_check = request_eph_data();
    if (eph_check == -1) {
      close(eph_fd);
      pthread_exit(nullptr);
    } else if (eph_check == 0) {
      break;
    }
    usleep(reqst_interval);
  }
  std::cout << "Initial EPH success" << std::endl;
  struct timezone tz({0, 0});
  // Log Clock and Orbit date.
  timeval tv{};
  uint64_t file_t;
  gettimeofday(&tv, &tz);
  file_t = get_sec(tv);
  eph_ready = true;
  periodic_info_t eph_prd{};
  make_periodic(CHECK_PERIOD, eph_prd);
  while (!eph_done) {
    gettimeofday(&tv, &tz);
    uint64_t now = get_sec(tv);
    // update log file
    if (now / FILE_PERIOD != file_t / FILE_PERIOD) {
      log_eph.close();
      eph_logpath[2] = eph_logpath[1];
      eph_logpath[1] = eph_logpath[0];
      eph_logpath[0] = FILE_PATH + "requestor_EPH_" + get_time_log() + ".log";
      log_eph.open(eph_logpath[0].c_str());
      if (!log_eph.is_open()) {
        fprintf(stderr, "requestor new EPH log file cannot be opened\n");
      }
      if (!eph_logpath[2].empty()) {
        remove(eph_logpath[2].c_str());  // Remove file from two days before
      }
      file_t = now;
    }
    // Keep read port untill no data update
    while (true) {
      int eph_check = request_eph_data();
      if (eph_check == -1) {
        close(eph_fd);
        pthread_exit(nullptr);
      } else if (eph_check == 0) {
        break;
      }
      usleep(reqst_interval);
    }
    wait_period(&eph_prd);
  }
}

void requestor_BKG::request_ssr() {
  if (!bkg_socket_client(SSR_port, local_ip, ssr_fd)) {
    std::cerr << "Please check if the igs correction port enable" << std::endl;
    pthread_exit(nullptr);
  }
  // keep requesting until the latest version
  while (true) {
    int IGS_check = request_ssr_data();
    if (IGS_check == -1) {
      close(ssr_fd);
      pthread_exit(nullptr);
    } else if (IGS_check > 0) {
      break;
    }
    usleep(reqst_interval);
  }
  std::cout << "Initial SSR success" << std::endl;
  struct timezone tz({0, 0});
  // Log Clock and Orbit date.
  timeval tv{};
  uint64_t log_t, file_t;
  gettimeofday(&tv, &tz);
  write_ssr_to_log();
  log_t = file_t = get_sec(tv);
  ssr_ready = true;
  periodic_info_t ssr_prd{};
  make_periodic(CHECK_PERIOD, ssr_prd);
  while (!ssr_done) {
    gettimeofday(&tv, &tz);
    uint64_t now = get_sec(tv);
    // Request USTEC data begin at HH:MM, HH:05 HH:20 HH:35 HH:50.
    if (now / SSRLOG_PERIOD != log_t / SSRLOG_PERIOD) {
      write_ssr_to_log();
      log_t = now;
    }
    // update log file
    if (now / FILE_PERIOD != file_t / FILE_PERIOD) {
      log_ssr.close();
      ssr_logpath[2] = ssr_logpath[1];
      ssr_logpath[1] = ssr_logpath[0];
      ssr_logpath[0] = FILE_PATH + "requestor_SSR_" + get_time_log() + ".log";
      log_ssr.open(ssr_logpath[0].c_str());
      if (!log_ssr.is_open()) {
        fprintf(stderr, "requestor new SSR log file cannot be opened\n");
      }
      if (!ssr_logpath[2].empty()) {
        remove(ssr_logpath[2].c_str());  // Remove file from two days before
      }
      file_t = now;
    }
    if (request_ssr_data() == -1) {
      close(ssr_fd);
      pthread_exit(nullptr);
    }
    // Keep read port untill no data update
    while (true) {
      int eph_check = request_eph_data();
      if (eph_check == -1) {
        close(ssr_fd);
        pthread_exit(nullptr);
      } else if (eph_check == 0) {
        break;
      }
      usleep(reqst_interval);
    }
    wait_period(&ssr_prd);
  }
}

// Wrapper for request function
void *requestor_BKG::request_eph_wrapper(void *arg) {
  reinterpret_cast<requestor_BKG *>(arg)->request_eph();
  return nullptr;
}
void *requestor_BKG::request_ssr_wrapper(void *arg) {
  reinterpret_cast<requestor_BKG *>(arg)->request_ssr();
  return nullptr;
}

std::vector<ssr_orbit> requestor_BKG::get_orbit_data() {
  std::lock_guard<std::mutex> lock(obt_mutex);
  return obt_data;
}

std::vector<ssr_clock> requestor_BKG::get_clock_data() {
  std::lock_guard<std::mutex> lock(clk_mutex);
  return clk_data;
}

std::vector<eph_struct> requestor_BKG::get_eph_data() {
  std::lock_guard<std::mutex> lock(eph_mutex);
  return eph_data;
}

vtec_t requestor_BKG::get_vtec_data() {
  std::lock_guard<std::mutex> lock(tec_mutex);
  return vtec_data;
}

cbias_ssr requestor_BKG::get_code_bias() {
  std::lock_guard<std::mutex> lock(cbs_mutex);
  return code_bias;
}

pbias_ssr requestor_BKG::get_phase_bias() {
  std::lock_guard<std::mutex> lock(pbs_mutex);
  return phase_bias;
}

// start data request
void requestor_BKG::start_request() {
  //  while (!update_stream())
  //    ;
  eph_done = false;
  ssr_done = false;
  eph_ready = false;
  ssr_ready = false;
  eph_logpath[0] = FILE_PATH + "requestor_EPH_" + get_time_log() + ".log";
  ssr_logpath[0] = FILE_PATH + "requestor_SSR_" + get_time_log() + ".log";
  log_eph.open(eph_logpath[0].c_str());
  log_ssr.open(ssr_logpath[0].c_str());
  if (!(log_eph.is_open() && log_ssr.is_open())) {
    fprintf(stderr, "BKG requestor log file cannot be opened\n");
  }
  // Resize eph_data to 10 versions
  eph_data.resize(EPH_VERMAX);
  clk_data.resize(3);
  obt_data.resize(3);
  pthread_create(&pid_eph, nullptr, request_eph_wrapper, this);
  pthread_create(&pid_ssr, nullptr, request_ssr_wrapper, this);
  while (!(eph_ready && ssr_ready)) {
    sleep(2);
  }
}

// end data request
void requestor_BKG::end_request() {
  eph_done = true;
  ssr_done = true;
  pthread_join(pid_eph, nullptr);
  pthread_join(pid_ssr, nullptr);
  log_eph.close();
  log_ssr.close();
}
