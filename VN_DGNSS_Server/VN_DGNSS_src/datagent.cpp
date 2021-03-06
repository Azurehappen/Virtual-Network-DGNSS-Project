#include "datagent.h"
#include <iomanip>
#include <iostream>
#include <utility>
#include "SSR_VTEC.h"
static void out_to_file_datetime(std::ostream &rst, std::vector<double> datetime) {
  rst << std::setfill('0')<<std::setw(4)<< (int) datetime[0] << " "
      << std::setfill('0')<<std::setw(2)<< (int) datetime[1] << " "
      << std::setfill('0')<<std::setw(2)<< (int) datetime[2] << " "
      << std::setfill('0')<<std::setw(2)<< (int) datetime[3] << " "
      << std::setfill('0')<<std::setw(2)<< (int) datetime[4] << " "
      << std::setfill('0')<<std::setw(2)<< (int) datetime[5] << std::endl;
}

static std::string sys_string(int sys) {
  if (sys == SYS_GPS) {
    return "G";
  } else if (sys == SYS_GAL) {
    return "E";
  } else if (sys == SYS_CMP) {
    return "C";
  } else {
    return "unknown";
  }
}

static int infor_to_rtcm_code(int info_code, int sys,int prn) {
  switch (sys) {
  case SYS_GPS: {
    switch (info_code) {
    case code_GPS_C1C:
      return CODE_L1C;
    case code_GPS_C1W:
      return CODE_L1W;
    case code_GPS_C2C:
      return CODE_L2C;
    case code_GPS_C2W:
      return CODE_L2W;
    case code_GPS_C2L:
      return CODE_L2L;
    default: return CODE_NONE;
    }
  }
  case SYS_GAL: {
    switch (info_code) {
    case code_GAL_C1C:
      return CODE_L1C;
    case code_GAL_C1X:
      return CODE_L1X;
    case code_GAL_C6C:
      return CODE_L6C;
    case code_GAL_C5Q:
      return CODE_L5Q;
    case code_GAL_C5X:
      return CODE_L5X;
    case code_GAL_C7Q:
      return CODE_L7Q;
    case code_GAL_C7X:
      return CODE_L7X;
    default: return CODE_NONE;
    }
  }
  case SYS_CMP: {
    switch (info_code) {
    case code_BDS_C2I:
      return CODE_L2I;
    case code_BDS_C6I:
      return CODE_L6I;
    case code_BDS_C7:
      if (prn <= 18) {
        return CODE_L7I;
      } else {
        return CODE_L7Z;
      }
    default: return CODE_NONE;
    }
  }
  default: return CODE_NONE;
  }
}

datagent::datagent(std::vector<double> pos_ecef)
    : user_pos(std::move(pos_ecef)),
      date_gps(6, 0) {}

datagent::~datagent() = default;

void datagent::getgpstnow() {
  get_gpst_now(date_gps, doy,
               gpst_now); // get current GPS time and days of year
}

void datagent::getcorrdata(requestor_BKG *foo_bkg, requestor_web *foo_web,
                           std::ostream &rst) {
  // get USTEC ionospheric data
  ustec_data = foo_web->get_ustec_data();
  // get clock correction data
  clock_data = foo_bkg->get_clock_data();
  // get orbit correction data
  orbit_data = foo_bkg->get_orbit_data();
  // get hardware bias
  code_bias = foo_web->get_code_bias();
  code_bias_ssr = foo_bkg->get_code_bias();
  phase_bias_ssr = foo_bkg->get_phase_bias();
  // phase_bias = foo_web->get_phase_bias();
  // get ephemeris data
  eph_data = foo_bkg->get_eph_data();
  vtec_CNE = foo_bkg->get_vtec_data();
}

// Find orbit data that match the selected PRN.
bool datagent::orbit_pick(std::ostream &rst, int prn, int sys,
                          orbit_element &obt_sv, gtime_t &obt_t) {
  for (int i = 0;i<3;i++) {
    orbit_element obt_elem;
    gtime_t ssrt_sys;
    switch (sys) {
    case SYS_GPS:
      obt_elem = orbit_data[i].GPS.data_sv[prn];
      ssrt_sys = orbit_data[i].GPS.time;
      break;
    case SYS_GAL:
      obt_elem = orbit_data[i].GAL.data_sv[prn];
      ssrt_sys = orbit_data[i].GAL.time;
      break;
    case SYS_CMP:
      obt_elem = orbit_data[i].BDS.data_sv[prn];
      ssrt_sys = orbit_data[i].BDS.time;
      break;
    default: obt_elem.prn = -1;
    }
    if (obt_elem.prn != -1 &&
        timediff(gpst_now, ssrt_sys) < 200.0) {
      obt_sv = obt_elem;
      obt_t = ssrt_sys;
      return true;
    }
  }
  // No data matching
  return false;
}

bool datagent::clock_pick(std::ostream &rst, int prn, int sys,
                          clock_element &clk_sv, gtime_t &clk_t) {
  for (int i = 0;i<3;i++) {
    clock_element clk_elem;
    gtime_t ssrt_sys;
    switch (sys) {
    case SYS_GPS:
      clk_elem = clock_data[i].GPS.data_sv[prn];
      ssrt_sys = clock_data[i].GPS.time;
      break;
    case SYS_GAL:
      clk_elem = clock_data[i].GAL.data_sv[prn];
      ssrt_sys = clock_data[i].GAL.time;
      break;
    case SYS_CMP:
      clk_elem = clock_data[i].BDS.data_sv[prn];
      ssrt_sys = clock_data[i].BDS.time;
      break;
    default: clk_elem.prn = -1;
    }
    if (clk_elem.prn != -1 &&
        timediff(gpst_now, ssrt_sys) < 200.0) {
      clk_sv = clk_elem;
      clk_t = ssrt_sys;
      return true;
    }
  }
  // No data matching
  return false;
}

bool datagent::computemeasrements(requestor_BKG *foo_bkg, requestor_web *foo_web,
                                  std::ostream &rst, const sys_infor& infor,
                                  const IGGexpModel& TropData,
                                  std::vector<std::vector<double>> &phw_track,
                                  int log_count) {
  getgpstnow();
  bool log_out = false;
  if (log_count%60 == 1) { // record log by every 1 minute
    log_out = true;
  }
  if (log_out){
    rst << "current GPS time: " << date_gps[0] << " " << date_gps[1] << " "
        << date_gps[2] << " " << date_gps[3] << " " << date_gps[4] << " "
        << date_gps[5] << std::endl;
  }
  getcorrdata(foo_bkg, foo_web, rst);
  double tdiff;
  /* Mute USTEC
  gtime_t t_ustec = epoch2time(ustec_data.time);
  tdiff = difftime(gpst_now.time, t_ustec.time) - 18;
  if (log_out) {
    rst << "USTEC t_diff: " << (int)(tdiff / 60) << ":"
        << tdiff - 60 * (int)(tdiff / 60) << std::endl;
    if (tdiff > 45 * 60) {
      rst << "warning: USTEC too old (>45min)." << std::endl;
    }
  }
  */

  tdiff = difftime(gpst_now.time, vtec_CNE.time.time);
  if (log_out) {
    rst << "SSR TEC t_diff: " << (int)(tdiff / 60) << ":"
        << tdiff - 60 * (int)(tdiff / 60) << std::endl;
    if (tdiff > 10 * 60) {
      rst << "warning: SSR TEC too old (>10min)." << std::endl;
      return false;
    }
  }

  if (vtec_CNE.check) {
    if (log_out) {
      rst << "SSR VTEC used." << std::endl;
    }
  }

  double user_lat = 0;
  double user_lon = 0;
  double user_h = 0;
  /*  Mute USTEC
  IonoDelay ido(ustec_data.data, user_pos);
  ido.GetLatLonHeight(user_lat, user_lon, user_h);
  */
  // Get LLA in rad
  double LLA[3]; // Lat,Lon,H
  ecef2pos(user_pos.data(),LLA);
  user_lat = LLA[0];
  user_lon = LLA[1];
  geoid geoH;
  // geodetic ellipsoidal separation, compute orthometric height of the receiver
  double Ngeo = geoH.geoidh(user_lat, user_lon);
  user_h = LLA[2] - Ngeo;

  int sys = SYS_NONE;
  int max_prn = 0;
  // Initial time gap check
  int t_check=0;
  if (timediff(gpst_now, clock_data[0].GPS.time)>200 ||
      timediff(gpst_now, orbit_data[0].GPS.time)>200) {
    if (log_out) {
      rst << "GPS SSR data too old (>200s)" << std::endl;
    }
    t_check++;
  }
  if (timediff(gpst_now, clock_data[0].GAL.time)>200 ||
      timediff(gpst_now, orbit_data[0].GAL.time)>200) {
    if (log_out) {
      rst << "GAL SSR data too old (>200s)" << std::endl;
    }
    t_check++;
  }
  if (timediff(gpst_now, clock_data[0].BDS.time)>200 ||
      timediff(gpst_now, orbit_data[0].BDS.time)>200) {
    if (log_out) {
      rst << "BDS SSR data too old (>200s)" << std::endl;
    }
    t_check++;
  }
  if (t_check == 3) {
    return false;
  }
  // Generate and initialize data struct for RTCM
  data.resize(MAXOBS);
  for (int i = 0; i < MAXOBS; i++) {
    for (int k = 0; k < NFREQ + NEXOBS; k++) {
      data[i].P[k] = data[i].L[k] = 0.0;
      data[i].D[k] = 0.0f;
      data[i].SNR[k] = data[i].LLI[k] = data[i].code[k] = 0;
      data[i].lockt[k] = 0;
    }
  }
  num_sv = 0;
  num_in_sys.resize(3,0);
  for (int sys_i = 0; sys_i < 3; sys_i++) {
    // Checking if the corresponding system requested by client
    if (infor.sys[sys_i] && infor.code_F1[sys_i] != -1) {
      std::vector<bias_element> cbias_sv1, cbias_sv2;
      cbias_ver cbias_sv;
      pbias_ver pbias_sv;
      std::vector<SatPosClkComp::Ephemeris> eph_sv[EPH_VERMAX];
      double sys_F1, sys_F2;
      switch (sys_i) {
      case 0:
        sys = SYS_GPS;
        max_prn = MAXPRNGPS;
        cbias_sv = code_bias_ssr.GPS;
        pbias_sv = phase_bias_ssr.GPS;
        cbias_sv1 = code_bias.cbias_GPS[infor.code_F1[sys_i]]; // GIPP data
        cbias_sv2 = code_bias.cbias_GPS[infor.code_F2[sys_i]]; // GIPP data
        for (int j=0;j<EPH_VERMAX;j++) { // Copy different version
          eph_sv[j] = eph_data[j].GPS_eph;
        }
        sys_F1 = FREQL1;
        sys_F2 = FREQL2;
        break;
      case 1:
        sys = SYS_GAL;
        max_prn = MAXPRNGAL;
        cbias_sv = code_bias_ssr.GAL;
        pbias_sv = phase_bias_ssr.GAL;
        cbias_sv1 = code_bias.cbias_GAL[infor.code_F1[sys_i]];
        cbias_sv2 = code_bias.cbias_GAL[infor.code_F2[sys_i]];
        for (int j=0;j<EPH_VERMAX;j++) {
          eph_sv[j] = eph_data[j].GAL_eph;
        }
        sys_F1 = FREQL1;
        sys_F2 = FREQE5b;
        break;
      case 2:
        sys = SYS_CMP;
        max_prn = MAXPRNCMP;
        cbias_sv = code_bias_ssr.BDS;
        pbias_sv = phase_bias_ssr.BDS;
        cbias_sv1 = code_bias.cbias_BDS[infor.code_F1[sys_i]];
        cbias_sv2 = code_bias.cbias_BDS[infor.code_F2[sys_i]];
        for (int j=0;j<EPH_VERMAX;j++) {
          eph_sv[j] = eph_data[j].BDS_eph;
        }
        sys_F1 = FREQ1_CMP;
        sys_F2 = FREQ2_CMP;
        break;
      }
      clock_element clk_sv;
      orbit_element obt_sv;
      gtime_t t_clk{},t_obt{};
      for (int prn = 1; prn < max_prn + 1; prn++) {
        if (sys_i == 2) {
          if (prn<=5||prn==18||prn>=59) {
            if (log_out) {
              rst << sys_string(sys) << prn << " BDS GEO SAT ignored" << std::endl;
            }
            phw_track[sys_i][prn] = 0;
            continue;
          }
        }
        // Pick SSR clock and orbit correction, check the latency
        if (! (orbit_pick(rst,prn,sys,obt_sv,t_obt)
            && clock_pick(rst,prn,sys,clk_sv,t_clk)) ) {
          if (log_out) {
            rst << sys_string(sys) << prn << " No IGS corr" << std::endl;
          }
          phw_track[sys_i][prn] = 0;
          continue;
        }
        if (cbias_sv1[prn].prn == -1) {
          if (log_out) {
            rst << sys_string(sys) << prn << " No code bias corr for freq 1" << std::endl;
          }
          continue;
        }
        int ver = -1;
        for (int j=0;j<EPH_VERMAX;j++) { // Checking for different eph version
          if (obt_sv.IOD==eph_sv[j][prn].IODE && eph_sv[j][prn].prn != -1) {
            ver = j;
            break;
          }
        }
        if (ver == -1) {
          if (log_out) {
            rst << sys_string(sys) << prn << " IOD not match: " << obt_sv.IOD
                << " EPH_IOD: ";
            for (int j = 0; j < 6; j++) {
              rst << eph_sv[j][prn].IODE << " ";
            }
            rst << std::endl;
          }
          phw_track[sys_i][prn] = 0;
          continue;
        }
        double eph_tdiff = timediff(gpst_now,eph_sv[ver][prn].t_oc);
        double ep[6];
        time2epoch(eph_sv[ver][prn].t_oc,ep);

        if (abs(eph_tdiff) > 7200.0 + 120.0 || eph_sv[ver][prn].svH != 0 ||
            eph_sv[ver][prn].prn == -1) {
          if (log_out) {
            rst << sys_string(sys) << prn << "(" << eph_sv[ver][prn].prn << ")"
                << " sv_H:" << eph_sv[ver][prn].svH << " time diff: "
                << eph_tdiff << std::endl;
          }
          phw_track[sys_i][prn] = 0;
          //record_0(phw_gps_track, prn_idx);
          continue;
        }
        SatPosClkComp spco(gpst_now, obt_sv.dx, obt_sv.dv, t_obt, clk_sv.dt_corr,
                           t_clk, eph_sv[ver][prn],sys);
        // compute propagation time using optimization function
        spco.PropTimeOptm(user_pos);
        // Rotate satellite position
        spco.ComputePreciseOrbitClockCorrection();
        // get precise satellite position
        std::vector<double> sat_pos_precise = spco.GetPreciseSatPos();
        // get precise satellite position at its transmit time
        std::vector<double> sat_pos_pretrans = spco.GetPreSatPosAtTranst();
        // get precise satellite clock bias
        double delt_sv = spco.GetClock();
        std::vector<double> range_vector(3, 0);
        for (int j=0;j<3;j++) {
          range_vector[j] = user_pos[j] - sat_pos_precise[j];
        }
        double norm_range = sqrt(pow(range_vector[0], 2) +
                                 pow(range_vector[1], 2) +
                                 pow(range_vector[2], 2));

        IonoDelay ido(ustec_data.data, user_pos);
        std::vector<double> elaz = ido.ElevationAzimuthComputation(sat_pos_precise);
        double user_elev = elaz[0];
        //int vtec_check = ido.VerticalTECComputation(sat_pos_precise);
        //double user_elev = ido.GetElevation();
        // if (vtec_check == -1) {
          if (user_elev <= ELEVMASK) {
//            rst << sys_string(sys) << prn << " elev: " << user_elev << std::endl;
          } else {
            if (log_out) {
              rst << sys_string(sys) << prn << " can't compute Iono corr" << std::endl;
            }
          }
        //  phw_track[sys_i][prn] = 0;
        //  continue;
        // }

        // compute ionospheric delay
        double iono_delay_L1 = 0, iono_delay_L2 = 0;
        // If SSR VTEC not avaliable, then using USTEC
        if (vtec_CNE.check) {
          SSR_VTEC VTEC;
          iono_delay_L1 = VTEC.stec(vtec_CNE,gpst_now.sec,user_pos,sat_pos_precise,sys_F1);
          iono_delay_L2 =
              iono_delay_L1 * (sys_F1 * sys_F1 / (sys_F2 * sys_F2));
        } else {
          if (log_out) {
            rst << sys_string(sys) << prn << " SSR VTEC data not avaliable" << std::endl;
          }
          /*  // Mute USTEC
          double vtec = ido.GetVTEC();
          double stec = ido.SlantTECComputation(vtec);
          iono_delay_L1 = ido.TEC2IonoDelayComputation(stec, sys_F1);
          iono_delay_L2 =
              iono_delay_L1 * (sys_F1 * sys_F1 / (sys_F2 * sys_F2));
          */
        }

        // compute Tropospheric delay
        IGGtrop IGG;
        double uLon;
        if (user_lon<=0){
          uLon = -user_lon;
        } else{
          uLon = -user_lon+2*PI;
        }
        double trop_IGG = IGG.IGGtropdelay(uLon*R2D,user_lat*R2D,user_h/1000,
                                           doy,user_elev,TropData.data);

        if (isnan(norm_range)) {
          rst << "sat prc pos rotated: " << std::setprecision(13) << " "
              << sat_pos_precise[0] << " " << sat_pos_precise[1] << " "
              << sat_pos_precise[2] << std::endl;
          rst << "dx[0] ,dv[0],dt_corr[0]: " << obt_sv.dx[0]
              << " " << obt_sv.dv[0] << clk_sv.dt_corr[0] <<std::endl;
          rst << "timedoff now to ssr: "<<timediff(gpst_now,t_obt)
              <<" "<<timediff(gpst_now,t_clk) <<std::endl;
        }
        data[num_sv].sat = satno(sys,prn);
        data[num_sv].time = gpst_now;
        data[num_sv].P[0] = norm_range - delt_sv +
                      CLIGHT * cbias_sv1[prn].value * 1e-9+
                      iono_delay_L1 + trop_IGG;
        int intL1 = 15;
        data[num_sv].L[0] = 0;
        data[num_sv].SNR[0] = (unsigned char)(floor(72 * user_elev / (3 * PI)) + 41) * 4;
        if (data[num_sv].SNR[0] > 200) {data[num_sv].SNR[0] = 200;}
        data[num_sv].code[0] = infor_to_rtcm_code(infor.code_F1[sys_i],sys,prn);
        data[num_sv].rcv = 0;
        data[num_sv].lockt[0] = 0; // Set lock time to 0 aviod receiver using phase
        if (log_out) {
          rst << sys_string(sys) << prn
              << " Eph_diff: " << std::setprecision(5) << eph_tdiff
              << " IODE " << eph_sv[ver][prn].IODE
              << " L1 code: " << std::setprecision(12) << data[num_sv].P[0]
              << " IGGTrop: " << std::setprecision(4) << trop_IGG
              << " Iono: " << std::setprecision(5) << iono_delay_L1
              //<< " phw: " << setprecision(6) << phw
              << std::endl;
        }
        num_in_sys[sys_i]++;
        num_sv++;
      }
    } else {
      rst << infor.sys[sys_i] << " " << infor.code_F1[sys_i] << std::endl;
    }
  }
  if (num_sv > 3) {
    if (log_out) {
      rst << "GPS: " << num_in_sys[0] << " GAL: " << num_in_sys[1]
          << " BDS: " << num_in_sys[2] << std::endl;
    }
    return true;
  } else {
    if (log_out) {
      rst << "No. of sat <= 3" << std::endl;
    }
    return false;
  }
}


void datagent::sendRTCM(SockRTCM *client_info) {
  if (num_sv > 3) {
    int type[16];
    int m = 1; /*Number of OBS message type*/
    type[0] = 1005;
    if (num_in_sys[0]>0) {
      type[m++] = 1074; /* GPS massage type */
    }
    if (num_in_sys[1]>0) {
      type[m++] = 1094; /* GAL massage type */
    }
    if (num_in_sys[2]>0) {
      type[m++] = 1124; /* BDS massage type */
    }
    data2rtcm(num_sv, type, m, client_info, user_pos, data); /* Generate RTCM message */
  }
}
