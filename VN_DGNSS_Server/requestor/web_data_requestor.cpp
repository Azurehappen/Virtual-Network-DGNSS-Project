
#include "web_data_requestor.h"

// Write function for curl
size_t WebDataRequestor::WriteToBuffer(void *ptr, size_t size, size_t nmemb,
                                       void *userdata) {
  size_t n = size * nmemb;
  ((std::string *)userdata)->append((char *)ptr, n);
  return n;
}

// Write website compressed file to local file
size_t WebDataRequestor::WriteToCompressFile(void *buffer, size_t size,
                                             size_t nmemb, void *stream) {
  auto fileout = (FtpFile *)stream;
  if (fileout && !fileout->fp) {  // aviod reopen file
    fileout->fp = fopen(fileout->filepath, "wb");
    if (!fileout->fp) {
      return -1;
    }
  }
  return fwrite(buffer, size, nmemb, fileout->fp);
}

bool WebDataRequestor::DownloadFile(const char *url, const char *file_path) {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    FtpFile fileout{};
    fileout.filepath = file_path;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToCompressFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileout);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kTimeoutForCurl);
    res = curl_easy_perform(curl);
    if (fileout.fp) {
      fclose(fileout.fp);
    } else {
      return false;
    }
    if (res != CURLE_OK) {
      log_web << vntimefunc::GetLocalTimeString()
              << " curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return false;
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return true;
  } else {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return false;
  }
}

bool WebDataRequestor::ReadWebpage(const char *url, std::string &buffer) {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    // 1. Read name of file
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kTimeoutForCurl);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      log_web << vntimefunc::GetLocalTimeString()
              << "Bias: curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return false;
    } else {
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return true;
    }
  } else {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return false;
  }
}

// Clear stringstream and input line string
void WebDataRequestor::ClearInputStream(std::stringstream &ss,
                                        std::string &line) {
  ss.clear();
  ss.str("");
  ss << line;
}

// Establish connection to kUsTecCorrectionUrl and write the data to buffer
bool WebDataRequestor::RequestUsTecData() {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    std::string buffer, line;
    curl_easy_setopt(curl, CURLOPT_URL, kUsTecCorrectionUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kTimeoutForCurl);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      log_web << vntimefunc::GetLocalTimeString()
              << "USTEC curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    } else {
      std::stringstream ifs(buffer);
      UsTecCorrData new_ustec_data;
      std::vector<std::vector<int>> grid_map;
      while (getline(ifs, line)) {
        // ignore header
        if (line[0] == ':') {
          if (line.substr(1, 7) == "Product") {
            if (line == ustec_fname) {
              curl_easy_cleanup(curl);
              curl_global_cleanup();
              // log_ofs << GetLocalTimeString() << "USTEC not update, back" <<
              // std::endl;
              return false;
            }
            ustec_fname = line;
            std::string type;
            size_t time;
            std::stringstream ss(line);
            ss >> type >> time;
            for (int i = 4; i >= 0; i--) {
              size_t v;
              if (i == 0) {
                v = time % 10000;  // four digits
                time /= 10000;
              } else {
                v = time % 100;  // two digits
                time /= 100;
              }
              new_ustec_data.time[i] = v;
            }
            new_ustec_data.time[5] = 0;
          }
          continue;
        } else if (line.empty() || line[0] == '#') {
          continue;
        }
        std::stringstream ss(line);
        std::vector<int> row;
        int val;
        while (ss >> val) {
          row.push_back(val);
        }
        new_ustec_data.data.push_back(row);
        // Only the first USETEC_NUM_ROW lines are needed
        if (new_ustec_data.data.size() >= kUsTecNumRow) {
          break;
        }
      }
      std::lock_guard<std::mutex> lock(ustec_mutex);
      ustec_data = new_ustec_data;
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    log_web << vntimefunc::GetLocalTimeString() << "received USTEC data"
            << std::endl;
    return true;
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return false;
}

// Read line GPS data
void WebDataRequestor::ReadGpsBiasCorr(int sv_prn, double value,
                                       const std::string &chn_type,
                                       BiasCorrData &bias) {
  if (chn_type.find("C1C") != std::string::npos) {
    bias.bias_GPS[VN_CODE_GPS_C1C][sv_prn].prn = sv_prn;
    bias.bias_GPS[VN_CODE_GPS_C1C][sv_prn].value = value;
  } else if (chn_type.find("C1W") != std::string::npos) {
    bias.bias_GPS[VN_CODE_GPS_C1W][sv_prn].prn = sv_prn;
    bias.bias_GPS[VN_CODE_GPS_C1W][sv_prn].value = value;
  } else if (chn_type.find("C2C") != std::string::npos) {
    bias.bias_GPS[VN_CODE_GPS_C2C][sv_prn].prn = sv_prn;
    bias.bias_GPS[VN_CODE_GPS_C2C][sv_prn].value = value;
  } else if (chn_type.find("C2W") != std::string::npos) {
    bias.bias_GPS[VN_CODE_GPS_C2W][sv_prn].prn = sv_prn;
    bias.bias_GPS[VN_CODE_GPS_C2W][sv_prn].value = value;
  } else if (chn_type.find("C2L") != std::string::npos) {
    bias.bias_GPS[VN_CODE_GPS_C2L][sv_prn].prn = sv_prn;
    bias.bias_GPS[VN_CODE_GPS_C2L][sv_prn].value = value;
  }
}

void WebDataRequestor::ReadGalBiasCorr(int sv_prn, double value,
                                       const std::string &chn_type,
                                       BiasCorrData &bias) {
  if (chn_type.find("C1C") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C1C][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C1C][sv_prn].value = value;
  } else if (chn_type.find("C1X") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C1X][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C1X][sv_prn].value = value;
  } else if (chn_type.find("C6C") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C6C][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C6C][sv_prn].value = value;
  } else if (chn_type.find("C5Q") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C5Q][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C5Q][sv_prn].value = value;
  } else if (chn_type.find("C5X") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C5X][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C5X][sv_prn].value = value;
  } else if (chn_type.find("C7Q") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C7Q][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C7Q][sv_prn].value = value;
  } else if (chn_type.find("C7X") != std::string::npos) {
    bias.bias_GAL[VN_CODE_GAL_C7X][sv_prn].prn = sv_prn;
    bias.bias_GAL[VN_CODE_GAL_C7X][sv_prn].value = value;
  }
}

// BDS-2 : prn 1-18; BDS-3: PRN 19-61
// B2 feq channel: C7I for BDS-2, C7Z for BDS-3
void WebDataRequestor::ReadBdsBiasCorr(int sv_prn, double value,
                                       const std::string &chn_type,
                                       BiasCorrData &cbias) {
  if (chn_type.find("C2I") != std::string::npos) {
    cbias.bias_BDS[VN_CODE_BDS_C2I][sv_prn].prn = sv_prn;
    cbias.bias_BDS[VN_CODE_BDS_C2I][sv_prn].value = value;
  } else if (chn_type.find("C6I") != std::string::npos) {
    cbias.bias_BDS[VN_CODE_BDS_C6I][sv_prn].prn = sv_prn;
    cbias.bias_BDS[VN_CODE_BDS_C6I][sv_prn].value = value;
  } else if (chn_type.find("C7I") != std::string::npos && sv_prn <= 18) {
    cbias.bias_BDS[VN_CODE_BDS_C7][sv_prn].prn = sv_prn;
    cbias.bias_BDS[VN_CODE_BDS_C7][sv_prn].value = value;
  } else if (chn_type.find("C7Z") != std::string::npos && sv_prn > 18) {
    cbias.bias_BDS[VN_CODE_BDS_C7][sv_prn].prn = sv_prn;
    cbias.bias_BDS[VN_CODE_BDS_C7][sv_prn].value = value;
  }
}

// Parse MGEX Bias-SINEX file
std::optional<BiasCorrData> WebDataRequestor::ParseBiasFromBuff(
    const std::vector<char> &buffer, const std::string &val_title) {
  std::string line;
  std::string buff_str(buffer.begin(), buffer.end());
  std::istringstream bias_stream(buff_str);
  // Start position of PRN, OBS TYPE, VALUE in the string
  int prn_st, type_st, value_st;
  bool find_data = false;
  // Read until to the data struct header
  while (std::getline(bias_stream, line)) {
    if (line.find("*BIAS SVN_ PRN") != std::string::npos) {
      prn_st = line.find("PRN");
      type_st = line.find("OBS1");
      value_st = line.find(val_title);
      find_data = true;
      break;
    }
  }
  if (!find_data) {
    return std::nullopt;
  }
  BiasCorrData new_bias;
  // Read bias data
  while (getline(bias_stream, line)) {
    if (line.find("-BIAS/SOLUTION") != std::string::npos) {
      break;
    }
    std::string PRN = line.substr(prn_st, sizeof("PRN"));
    std::string TYPE = line.substr(type_st, sizeof("OBS1"));
    std::string VALUE_s = line.substr(value_st, val_title.size());
    std::stringstream VALUE_ss(VALUE_s);
    double value;
    VALUE_ss >> value;
    if (PRN[0] == 'G') {
      int sv_prn = std::stoi(PRN.substr(1, line.size() - 1));
      ReadGpsBiasCorr(sv_prn, value, TYPE, new_bias);
    } else if (PRN[0] == 'E') {
      int sv_prn = std::stoi(PRN.substr(1, line.size() - 1));
      ReadGalBiasCorr(sv_prn, value, TYPE, new_bias);
    } else if (PRN[0] == 'C') {
      int sv_prn = std::stoi(PRN.substr(1, line.size() - 1));
      ReadBdsBiasCorr(sv_prn, value, TYPE, new_bias);
    }
  }
  return new_bias;
}

// Establish connection to CODE bias url and write the data to buffer
bool WebDataRequestor::RequestCodeBias() {
  int year = vntimefunc::GetCurrentYear();
  std::string CD_BIAS_URL =
      kGippCodeBiasCorrUrlHeader + std::to_string(year) + '/';
  std::string buffer{}, line, OSB_ss{};
  log_web << vntimefunc::GetLocalTimeString()
          << "read code bias data source list" << std::endl;
  if (!ReadWebpage(CD_BIAS_URL.c_str(), buffer)) {
    log_web << "read data list failed" << std::endl;
    return false;
  }
  std::stringstream ifs(buffer);
  while (ifs.peek() != EOF) {
    getline(ifs, line);
    if (line.find("OSB.BIA.gz") != std::string::npos) {
      OSB_ss = line;
    }
  }
  if (OSB_ss.empty()) {
    log_web << vntimefunc::GetLocalTimeString()
            << "get code bias file name failure: " << std::endl;
    return false;
  }
  std::string tmp, fname;
  std::stringstream ss(line);
  // detach the first 8 columns
  for (int i = 0; i < 8; ++i) {
    ss >> tmp;
  }
  ss >> fname;
  if (fname == code_bias_fname) {
    log_web << vntimefunc::GetLocalTimeString()
            << "get code bias file, but data not update" << std::endl;
    return true;
  }
  CD_BIAS_URL = CD_BIAS_URL + fname;
  log_web << vntimefunc::GetLocalTimeString()
          << "get code bias link: " << CD_BIAS_URL << std::endl;
  std::string data_path = file_path_ + "code_bias.BIA.gz";
  // 2. Download and uncompress code bias file
  if (!DownloadFile(CD_BIAS_URL.c_str(), data_path.c_str())) {
    log_web << vntimefunc::GetLocalTimeString() << "Download failed"
            << std::endl;
    return false;
  }
  gzFile fp = gzopen(data_path.c_str(), "rb");
  if (fp == nullptr) {
    log_web << vntimefunc::GetLocalTimeString() << "file not exist"
            << std::endl;
    return false;
  }
  std::vector<char> data;
  const int bufferSize = 4096;
  char sub_data_buff[bufferSize];
  int bytesRead;
  while ((bytesRead = gzread(fp, sub_data_buff, sizeof(sub_data_buff))) > 0) {
    data.insert(data.end(), sub_data_buff, sub_data_buff + bytesRead);
  }
  gzclose(fp);

  int n = remove(data_path.c_str());
  if (n == -1) {
    log_web << vntimefunc::GetLocalTimeString()
            << "remove code bias file failure: " << strerror(errno)
            << std::endl;
    return false;
  }
  if (data.empty()) {
    log_web << vntimefunc::GetLocalTimeString()
            << "No code bias data in the file" << std::endl;
    return false;
  }
  auto code_bias_data = ParseBiasFromBuff(data, "VALUE____");
  if (code_bias_data.has_value()) {
    std::lock_guard<std::mutex> lock(code_mutex);
    code_bias = code_bias_data.value();
  } else {
    log_web << vntimefunc::GetLocalTimeString() << "Parse code bias failure"
            << std::endl;
    return false;
  }
  code_bias_fname = fname;
  log_web << vntimefunc::GetLocalTimeString() << "received code bias"
          << std::endl;
  return true;
}

// Establish connection to PRIDE phase bias url and write the data to buffer
bool WebDataRequestor::RequestPhaseBias() {
  int year = vntimefunc::GetCurrentYear();
  std::string P_BIAS_URL =
      kWhuPhaseBiasCorrUrlHeader + std::to_string(year) + "/bias/";
  std::string buffer{}, line, OSB_ss{};
  log_web << vntimefunc::GetLocalTimeString()
          << "read phase bias data source list" << std::endl;
  if (!ReadWebpage(P_BIAS_URL.c_str(), buffer)) {
    log_web << "read data list failed" << std::endl;
    return false;
  }
  std::stringstream ifs(buffer);
  while (ifs.peek() != EOF) {
    getline(ifs, line);
    if (line.find("ABS.BIA.gz") != std::string::npos) {
      OSB_ss = line;
    }
  }
  if (OSB_ss.empty()) {
    log_web << vntimefunc::GetLocalTimeString()
            << "get phase bias file name failure: " << std::endl;
    return false;
  }
  std::string tmp, fname;
  std::stringstream ss(line);
  // detach the first 8 columns
  for (int i = 0; i < 8; ++i) {
    ss >> tmp;
  }
  ss >> fname;
  if (fname == phase_bias_fname) {
    log_web << vntimefunc::GetLocalTimeString()
            << "get phase bias file, but data not update" << std::endl;
    return true;
  }
  P_BIAS_URL = P_BIAS_URL + fname;
  log_web << vntimefunc::GetLocalTimeString()
          << "get phase bias link: " << P_BIAS_URL << std::endl;
  std::string data_path = file_path_ + "phase_bias.BIA.gz";
  // 2. Download and uncompress code bias file
  if (!DownloadFile(P_BIAS_URL.c_str(), data_path.c_str())) {
    log_web << vntimefunc::GetLocalTimeString() << "Download failed"
            << std::endl;
    return false;
  }
  gzFile fp = gzopen(data_path.c_str(), "rb");
  if (fp == nullptr) {
    log_web << vntimefunc::GetLocalTimeString() << "file not exist"
            << std::endl;
    return false;
  }
  std::vector<char> data;
  const int bufferSize = 4096;
  char sub_data_buff[bufferSize];
  int bytesRead;
  while ((bytesRead = gzread(fp, sub_data_buff, sizeof(sub_data_buff))) > 0) {
    data.insert(data.end(), sub_data_buff, sub_data_buff + bytesRead);
  }
  gzclose(fp);

  int n = remove(data_path.c_str());
  if (n == -1) {
    log_web << vntimefunc::GetLocalTimeString()
            << "remove phase bias file failure: " << strerror(errno)
            << std::endl;
    return false;
  }
  if (data.empty()) {
    log_web << vntimefunc::GetLocalTimeString()
            << "No phase bias data in the file" << std::endl;
    return false;
  }
  auto phase_bias_data = ParseBiasFromBuff(data, "__ESTIMATED_VALUE____");
  if (phase_bias_data.has_value()) {
    std::lock_guard<std::mutex> lock(phase_mutex);
    phase_bias = phase_bias_data.value();
  } else {
    log_web << vntimefunc::GetLocalTimeString() << "Parse phase bias failure"
            << std::endl;
    return false;
  }
  phase_bias_fname = fname;
  log_web << vntimefunc::GetLocalTimeString() << "received phase bias"
          << std::endl;
  return true;
}

// Wrapper for request function
void *WebDataRequestor::RequestWebWrapper(void *arg) {
  reinterpret_cast<WebDataRequestor *>(arg)->RequestWebData();
  return nullptr;
}

void WebDataRequestor::RequestWebData() {
  struct timezone tz({0, 0});
  timeval tv{};
  uint64_t ustec_t, code_t, phase_t, file_t;
  gettimeofday(&tv, &tz);
  ustec_t = code_t = phase_t = file_t = vntimefunc::GetSecFromTimeval(tv);
  timeperiodic::PeriodicInfoT periodic{};
  timeperiodic::MakePeriodic(kCheckUsTecPeriod, periodic);
  // keep requesting until success
  /*   //Mute USTEC
  while (!RequestUsTecData())
    ;
  std::cout << "Initial request USTEC success" << std::endl;
   */
  while (!RequestCodeBias())
    ;
  std::cout << "Initial request code bias success" << std::endl;
  while (!RequestPhaseBias())
    ;
  std::cout << "Initial request phase bias success" << std::endl;
  ready = true;
  while (!done) {
    gettimeofday(&tv, &tz);
    uint64_t now = vntimefunc::GetSecFromTimeval(tv);
    /*  // Mute USTEC
    // Request USTEC data begin at HH:MM, HH:05 HH:20 HH:35 HH:50.
    if (now / kUsTecPeriod != ustec_t / kUsTecPeriod) {
      RequestUsTecData();
      ustec_t = now;
    }
     */
    if (now / kBiasCorrUpdatePeriod != code_t / kBiasCorrUpdatePeriod) {
      log_web << vntimefunc::GetLocalTimeString() << "requesting code bias"
              << std::endl;
      if (RequestCodeBias()) {
        code_t = now;
      }
    }
    if (now / kBiasCorrUpdatePeriod != phase_t / kBiasCorrUpdatePeriod) {
      log_web << vntimefunc::GetLocalTimeString()
              << "requesting phase bias from PRIDE" << std::endl;
      if (RequestPhaseBias()) {
        phase_t = now;
      }
    }
    // update log file
    if (now / kBiasCorrUpdatePeriod != file_t / kBiasCorrUpdatePeriod) {
      log_web.close();
      log_path[2] = log_path[1];
      log_path[1] = log_path[0];
      log_path[0] = file_path_ + "requestor_WEB_" +
                    vntimefunc::GetLocalTimeStringForLog() + ".log";
      log_web.open(log_path[0].c_str());
      if (!log_web.is_open()) {
        fprintf(stderr, "requestor new WEB log file cannot be opened\n");
      }
      if (!log_path[2].empty()) {
        // Remove file from two days before
        remove(log_path[2].c_str());
        log_path->clear();
      }
      file_t = now;
    }
    timeperiodic::WaitPeriod(&periodic);
  }
  pthread_exit(nullptr);
}

// Return the lastest USTEC data
UsTecCorrData WebDataRequestor::get_ustec_data() {
  std::lock_guard<std::mutex> lock(ustec_mutex);
  return ustec_data;
}

BiasCorrData WebDataRequestor::get_code_bias() {
  std::lock_guard<std::mutex> lock(code_mutex);
  return code_bias;
}

BiasCorrData WebDataRequestor::get_phase_bias() {
  std::lock_guard<std::mutex> lock(phase_mutex);
  return phase_bias;
}

// start data request
void WebDataRequestor::StartRequest() {
  //  while (!update_stream())
  //    ;
  done = false;
  ready = false;
  log_path[0] = file_path_ + "requestor_WEB_" +
                vntimefunc::GetLocalTimeStringForLog() + ".log";
  log_web.open(log_path[0].c_str());
  if (!log_web.is_open()) {
    fprintf(stderr, "requestor log file cannot be opened\n");
  }
  pthread_create(&pid, nullptr, RequestWebWrapper, this);
  while (!ready) {
    sleep(2);
  }
}

// end data request
void WebDataRequestor::EndRequest() {
  done = true;
  pthread_join(pid, nullptr);
  log_web.close();
}
