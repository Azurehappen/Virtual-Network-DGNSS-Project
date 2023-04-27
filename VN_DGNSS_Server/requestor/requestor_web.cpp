
#include "requestor_web.h"

// Write function for curl
size_t requestor_web::write_to_buffer(void *contents, size_t size, size_t nmemb,
                                      void *userp) {
  size_t n = size * nmemb;
  ((std::string *)userp)->append((char *)contents, n);
  return n;
}

// Write website compressed file to local file
size_t requestor_web::write_to_compress_file(void *buffer, size_t size,
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

bool requestor_web::download_file(const char *url, const char *file_path) {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    FtpFile fileout{};
    fileout.filepath = file_path;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_compress_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileout);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    res = curl_easy_perform(curl);
    if (fileout.fp) {
      fclose(fileout.fp);
    } else {
      return false;
    }
    if (res != CURLE_OK) {
      log_WEB << get_time()
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

bool requestor_web::read_webpage(const char *url, std::string &buffer) {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    // 1. Read name of file
    std::string line, OSB_ss{};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      log_WEB << get_time()
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
void requestor_web::clear_input_stream(std::stringstream &ss,
                                       std::string &line) {
  ss.clear();
  ss.str("");
  ss << line;
}

// Establish connection to USTEC_URL and write the data to buffer
bool requestor_web::request_ustec_data() {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if (curl) {
    std::string buffer, line;
    curl_easy_setopt(curl, CURLOPT_URL, USTEC_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      log_WEB << get_time()
              << "USTEC curl_easy_perform() failed: " << curl_easy_strerror(res)
              << std::endl;
    } else {
      std::stringstream ifs(buffer);
      ustec new_ustec_data;
      std::vector<std::vector<int>> grid_map;
      while (getline(ifs, line)) {
        // ignore header
        if (line[0] == ':') {
          if (line.substr(1, 7) == "Product") {
            if (line == ustec_fname) {
              curl_easy_cleanup(curl);
              curl_global_cleanup();
              // log_ofs << get_time() << "USTEC not update, back" << std::endl;
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
        if (new_ustec_data.data.size() >= USTEC_NUM_ROW) {
          break;
        }
      }
      std::lock_guard<std::mutex> lock(ustec_mutex);
      ustec_data = new_ustec_data;
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    log_WEB << get_time() << "received USTEC data" << std::endl;
    return true;
  }
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return false;
}

// Read line GPS data
void requestor_web::ReadGpsBiasCorr(int sv_prn, double value,
                                    const std::string &TYPE,
                                    BiasCorrData &bias) {
  if (TYPE.find("C1C") != std::string::npos) {
    bias.bias_GPS[code_GPS_C1C][sv_prn].prn = sv_prn;
    bias.bias_GPS[code_GPS_C1C][sv_prn].value = value;
  } else if (TYPE.find("C1W") != std::string::npos) {
    bias.bias_GPS[code_GPS_C1W][sv_prn].prn = sv_prn;
    bias.bias_GPS[code_GPS_C1W][sv_prn].value = value;
  } else if (TYPE.find("C2C") != std::string::npos) {
    bias.bias_GPS[code_GPS_C2C][sv_prn].prn = sv_prn;
    bias.bias_GPS[code_GPS_C2C][sv_prn].value = value;
  } else if (TYPE.find("C2W") != std::string::npos) {
    bias.bias_GPS[code_GPS_C2W][sv_prn].prn = sv_prn;
    bias.bias_GPS[code_GPS_C2W][sv_prn].value = value;
  } else if (TYPE.find("C2L") != std::string::npos) {
    bias.bias_GPS[code_GPS_C2L][sv_prn].prn = sv_prn;
    bias.bias_GPS[code_GPS_C2L][sv_prn].value = value;
  }
}

void requestor_web::ReadGalBiasCorr(int sv_prn, double value,
                                    const std::string &TYPE,
                                    BiasCorrData &bias) {
  if (TYPE.find("C1C") != std::string::npos) {
    bias.bias_GAL[code_GAL_C1C][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C1C][sv_prn].value = value;
  } else if (TYPE.find("C1X") != std::string::npos) {
    bias.bias_GAL[code_GAL_C1X][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C1X][sv_prn].value = value;
  } else if (TYPE.find("C6C") != std::string::npos) {
    bias.bias_GAL[code_GAL_C6C][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C6C][sv_prn].value = value;
  } else if (TYPE.find("C5Q") != std::string::npos) {
    bias.bias_GAL[code_GAL_C5Q][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C5Q][sv_prn].value = value;
  } else if (TYPE.find("C5X") != std::string::npos) {
    bias.bias_GAL[code_GAL_C5X][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C5X][sv_prn].value = value;
  } else if (TYPE.find("C7Q") != std::string::npos) {
    bias.bias_GAL[code_GAL_C7Q][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C7Q][sv_prn].value = value;
  } else if (TYPE.find("C7X") != std::string::npos) {
    bias.bias_GAL[code_GAL_C7X][sv_prn].prn = sv_prn;
    bias.bias_GAL[code_GAL_C7X][sv_prn].value = value;
  }
}

// BDS-2 : prn 1-18; BDS-3: PRN 19-61
// B2 feq channel: C7I for BDS-2, C7Z for BDS-3
void requestor_web::ReadBdsBiasCorr(int sv_prn, double value,
                                    const std::string &TYPE,
                                    BiasCorrData &cbias) {
  if (TYPE.find("C2I") != std::string::npos) {
    cbias.bias_BDS[code_BDS_C2I][sv_prn].prn = sv_prn;
    cbias.bias_BDS[code_BDS_C2I][sv_prn].value = value;
  } else if (TYPE.find("C6I") != std::string::npos) {
    cbias.bias_BDS[code_BDS_C6I][sv_prn].prn = sv_prn;
    cbias.bias_BDS[code_BDS_C6I][sv_prn].value = value;
  } else if (TYPE.find("C7I") != std::string::npos && sv_prn <= 18) {
    cbias.bias_BDS[code_BDS_C7][sv_prn].prn = sv_prn;
    cbias.bias_BDS[code_BDS_C7][sv_prn].value = value;
  } else if (TYPE.find("C7Z") != std::string::npos && sv_prn > 18) {
    cbias.bias_BDS[code_BDS_C7][sv_prn].prn = sv_prn;
    cbias.bias_BDS[code_BDS_C7][sv_prn].value = value;
  }
}

// Parse MGEX Bias-SINEX file
std::optional<BiasCorrData> requestor_web::parse_code_bias(
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
bool requestor_web::request_code_bias() {
  int year = get_year();
  std::string CD_BIAS_URL = CODE_BIAS_URL_header + std::to_string(year) + '/';
  std::string buffer{}, line, OSB_ss{};
  log_WEB << get_time() << "read code bias data source list" << std::endl;
  if (!read_webpage(CD_BIAS_URL.c_str(), buffer)) {
    log_WEB << "read data list failed" << std::endl;
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
    log_WEB << get_time() << "get code bias file name failure: " << std::endl;
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
    log_WEB << get_time() << "get code bias file, but data not update"
            << std::endl;
    return true;
  }
  CD_BIAS_URL = CD_BIAS_URL + fname;
  log_WEB << get_time() << "get code bias link: " << CD_BIAS_URL << std::endl;
  std::string data_path = FILE_PATH + "code_bias.BIA.gz";
  // 2. Download and uncompress code bias file
  if (!download_file(CD_BIAS_URL.c_str(), data_path.c_str())) {
    log_WEB << get_time() << "Download failed" << std::endl;
    return false;
  }
  gzFile fp = gzopen(data_path.c_str(), "rb");
  if (fp == nullptr) {
    log_WEB << get_time() << "file not exist" << std::endl;
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
    log_WEB << get_time()
            << "remove code bias file failure: " << strerror(errno)
            << std::endl;
    return false;
  }
  if (data.empty()) {
    log_WEB << get_time() << "No code bias data in the file" << std::endl;
    return false;
  }
  auto code_bias_data = parse_code_bias(data, "VALUE____");
  if (code_bias_data.has_value()) {
    std::lock_guard<std::mutex> lock(code_mutex);
    code_bias = code_bias_data.value();
  } else {
    log_WEB << get_time() << "Parse code bias failure" << std::endl;
    return false;
  }
  code_bias_fname = fname;
  log_WEB << get_time() << "received code bias" << std::endl;
  return true;
}

// Establish connection to PRIDE phase bias url and write the data to buffer
bool requestor_web::request_phase_bias() {
  int year = get_year();
  std::string P_BIAS_URL =
      PHASE_BIAS_URL_header + std::to_string(year) + "/bias/";
  std::string buffer{}, line, OSB_ss{};
  log_WEB << get_time() << "read phase bias data source list" << std::endl;
  if (!read_webpage(P_BIAS_URL.c_str(), buffer)) {
    log_WEB << "read data list failed" << std::endl;
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
    log_WEB << get_time() << "get phase bias file name failure: " << std::endl;
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
    log_WEB << get_time() << "get phase bias file, but data not update"
            << std::endl;
    return true;
  }
  P_BIAS_URL = P_BIAS_URL + fname;
  log_WEB << get_time() << "get phase bias link: " << P_BIAS_URL << std::endl;
  std::string data_path = FILE_PATH + "phase_bias.BIA.gz";
  // 2. Download and uncompress code bias file
  if (!download_file(P_BIAS_URL.c_str(), data_path.c_str())) {
    log_WEB << get_time() << "Download failed" << std::endl;
    return false;
  }
  gzFile fp = gzopen(data_path.c_str(), "rb");
  if (fp == nullptr) {
    log_WEB << get_time() << "file not exist" << std::endl;
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
    log_WEB << get_time()
            << "remove phase bias file failure: " << strerror(errno)
            << std::endl;
    return false;
  }
  if (data.empty()) {
    log_WEB << get_time() << "No phase bias data in the file" << std::endl;
    return false;
  }
  auto phase_bias_data = parse_code_bias(data, "__ESTIMATED_VALUE____");
  if (phase_bias_data.has_value()) {
    std::lock_guard<std::mutex> lock(phase_mutex);
    phase_bias = phase_bias_data.value();
  } else {
    log_WEB << get_time() << "Parse phase bias failure" << std::endl;
    return false;
  }
  phase_bias_fname = fname;
  log_WEB << get_time() << "received phase bias" << std::endl;
  return true;
}

// Wrapper for request function
void *requestor_web::request_web_wrapper(void *arg) {
  reinterpret_cast<requestor_web *>(arg)->request_web_data();
  return nullptr;
}

void requestor_web::request_web_data() {
  struct timezone tz({0, 0});
  timeval tv{};
  uint64_t ustec_t, code_t, phase_t, file_t;
  gettimeofday(&tv, &tz);
  ustec_t = code_t = phase_t = file_t = get_sec(tv);
  periodic_info_t periodic{};
  make_periodic(CHECK_USTEC_PERIOD, periodic);
  // keep requesting until success
  /*   //Mute USTEC
  while (!request_ustec_data())
    ;
  std::cout << "Initial request USTEC success" << std::endl;
   */
  while (!request_code_bias())
    ;
  std::cout << "Initial request code bias success" << std::endl;
  while (!request_phase_bias())
    ;
  std::cout << "Initial request phase bias success" << std::endl;
  ready = true;
  while (!done) {
    gettimeofday(&tv, &tz);
    uint64_t now = get_sec(tv);
    /*  // Mute USTEC
    // Request USTEC data begin at HH:MM, HH:05 HH:20 HH:35 HH:50.
    if (now / USTEC_PERIOD != ustec_t / USTEC_PERIOD) {
      request_ustec_data();
      ustec_t = now;
    }
     */
    if (now / HW_PERIOD != code_t / HW_PERIOD) {
      log_WEB << get_time() << "requesting code bias" << std::endl;
      if (request_code_bias()) {
        code_t = now;
      }
    }
    if (now / HW_PERIOD != phase_t / HW_PERIOD) {
      log_WEB << get_time() << "requesting phase bias from PRIDE" << std::endl;
      if (request_phase_bias()) {
        phase_t = now;
      }
    }
    // update log file
    if (now / HW_PERIOD != file_t / HW_PERIOD) {
      log_WEB.close();
      log_path[2] = log_path[1];
      log_path[1] = log_path[0];
      log_path[0] = FILE_PATH + "requestor_WEB_" + get_time_log() + ".log";
      log_WEB.open(log_path[0].c_str());
      if (!log_WEB.is_open()) {
        fprintf(stderr, "requestor new WEB log file cannot be opened\n");
      }
      if (!log_path[2].empty()) {
        // Remove file from two days before
        remove(log_path[2].c_str());
        log_path->clear();
      }
      file_t = now;
    }
    wait_period(&periodic);
  }
  pthread_exit(nullptr);
}

// Return the lastest USTEC data
ustec requestor_web::get_ustec_data() {
  std::lock_guard<std::mutex> lock(ustec_mutex);
  return ustec_data;
}

BiasCorrData requestor_web::get_code_bias() {
  std::lock_guard<std::mutex> lock(code_mutex);
  return code_bias;
}

BiasCorrData requestor_web::get_phase_bias() {
  std::lock_guard<std::mutex> lock(phase_mutex);
  return phase_bias;
}

// start data request
void requestor_web::start_request() {
  //  while (!update_stream())
  //    ;
  done = false;
  ready = false;
  log_path[0] = FILE_PATH + "requestor_WEB_" + get_time_log() + ".log";
  log_WEB.open(log_path[0].c_str());
  if (!log_WEB.is_open()) {
    fprintf(stderr, "requestor log file cannot be opened\n");
  }
  pthread_create(&pid, nullptr, request_web_wrapper, this);
  while (!ready) {
    sleep(2);
  }
}

// end data request
void requestor_web::end_request() {
  done = true;
  pthread_join(pid, nullptr);
  log_WEB.close();
}
