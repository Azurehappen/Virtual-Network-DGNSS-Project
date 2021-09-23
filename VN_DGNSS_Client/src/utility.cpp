
#include "utility.h"

// Get local time string: [YYYY-MM-DD hh:mm:ss]
std::string local_tstr() {
  time_t rawtime;
  struct tm *ltm;
  time(&rawtime);
  ltm = localtime(&rawtime);
  char buf[28];
  sprintf(buf, "[%04d-%02d-%02d %02d:%02d:%02d]: ", ltm->tm_year + 1900,
          ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
          ltm->tm_sec);
  return std::string(buf);
}

// Get system seconds
#if WIN32
uint64_t get_sec(SYSTEMTIME tv) {
  return ((double)tv.wSecond) + ((double)tv.wMilliseconds) / 1000000.;
}
#else
uint64_t get_sec(timeval tv) {
  return ((double)tv.tv_sec) + ((double)tv.tv_usec) / 1000000.;
}
#endif

// Degree to Radius
double D2R(double D) { return D * PI / 180; }

/* transform LLA to ECEF position -------------------------------------
 * args   : LLA      geodetic position {lat,lon,alt} (rad,m)
 *          ECEF     ecef position {x,y,z} (m)
 * Datum  : WGS84
 * Alt: Ellipsoidal height
 *-------------------------------------------------------------------*/
void pos2ecef(std::vector<double> LLA, std::vector<double> &ECEF) {
  double sinp = sin(LLA[0]), cosp = cos(LLA[0]), sinl = sin(LLA[1]),
         cosl = cos(LLA[1]);
  double e2 = F_WGS84 * (2.0 - F_WGS84),
          v = R_WGS84 / sqrt(1.0 - e2 * sinp * sinp);
  ECEF[0] = (v + LLA[2]) * cosp * cosl;
  ECEF[1] = (v + LLA[2]) * cosp * sinl;
  ECEF[2] = (v * (1.0 - e2) + LLA[2]) * sinp;
}

// Decode GxGNS message
bool interpret_GNS(std::vector<double> &LLA, const std::string &GNSstr) {
  std::stringstream ss(GNSstr);
  std::string part;
  getline(ss, part, ',');  // $GxGNS
  getline(ss, part, ',');  // UTC time

  getline(ss, part, ',');  // Lat
  if (part.empty()) {return false;}
  std::string Lat_d_str = part.substr(0, 2);
  std::string Lat_m_str = part.substr(2);

  getline(ss, part, ',');  // North/South indicator
  std::string Lat_sig_str = part;

  getline(ss, part, ',');  // Lon
  std::string Lon_d_str = part.substr(0, 3);
  std::string Lon_m_str = part.substr(3);

  getline(ss, part, ',');  // East/West indicator
  std::string Lon_sig_str = part;

  for (int i = 0; i < 4; i++) {
    getline(ss, part, ',');  // posMode numSV HDOP alt(MSL)
  }
  std::string Alt_msl_str = part;
  getline(ss, part, ',');  // Geoid separation
  std::string sep = part;

  int Lat_sig = 0, Lon_sig = 0;
  if (Lat_sig_str == "N")
    Lat_sig = 1;
  else if (Lat_sig_str == "S")
    Lat_sig = -1;
  if (Lon_sig_str == "E")
    Lon_sig = 1;
  else if (Lon_sig_str == "W")
    Lon_sig = -1;
  if (stoi(Lat_d_str) == 0 && stoi(Lon_d_str) == 0 || Lat_sig_str.empty()) {
    return false;
  }
  LLA[0] = Lat_sig * (stod(Lat_d_str) + stod(Lat_m_str) / 60);  // Lat
  LLA[1] = Lon_sig * (stod(Lon_d_str) + stod(Lon_m_str) / 60);  // Lon
  LLA[2] = stod(Alt_msl_str) - stod(sep);  // Alt above ellipsoid
  return true;
}