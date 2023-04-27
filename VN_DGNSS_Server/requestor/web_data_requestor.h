#ifndef VN_DGNSS_SERVER_WEB_DATA_REQUESTOR_H
#define VN_DGNSS_SERVER_WEB_DATA_REQUESTOR_H
#pragma once
#include <arpa/inet.h>
#include <curl/curl.h>
#include <zlib.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>

#include "rtklib.h"
#include "time_common_func.h"
#include "constants.h"

// URL of USTEC data
static constexpr const char *kUsTecCorrectionUrl =
    "https://services.swpc.noaa.gov/text/"
    "us-tec-total-electron-content.txt";
// number of lines needed in USTEC data
static constexpr int kUsTecNumRow = 52;
// URL of CODE_BIAS header
static constexpr const char *kGippCodeBiasCorrUrlHeader =
    "ftp://ftp.gipp.org.cn/product/dcb/daybias/";
// URL of PHASE_BIAS header
static constexpr const char *kWhuPhaseBiasCorrUrlHeader =
    "ftp://igs.gnsswhu.cn/pub/whu/phasebias/";
// update period of USTEC data in seconds
static constexpr int kUsTecPeriod = 60;
// timeout parameter for curl function
static constexpr int kTimeoutForCurl = 60;
// update period of hardware bias in seconds
static constexpr int kBiasCorrUpdatePeriod = 86400;
// Check USTEC period in microseconds
static constexpr int kCheckUsTecPeriod = 60 * 1000000;

// Struct for download ftp file
struct FtpFile {
  const char *filepath;
  FILE *fp;
};

struct UsTecCorrData {
  double time[6]{};
  std::vector<std::vector<int>> data;
};

struct SatBias {
  // when not available, prn is -1
  int prn;
  double value;
};

struct BiasCorrData {
  std::vector<SatBias> bias_GPS[MAX_VN_CODE_GPS];
  std::vector<SatBias> bias_GAL[MAX_VN_CODE_GAL];
  std::vector<SatBias> bias_BDS[MAX_VN_CODE_BDS];
  BiasCorrData() {
    for (auto &i : bias_GPS) {
      i.resize(MAXPRNGPS + 1);
      for (int j = 0; j < MAXPRNGPS + 1; j++) {
        i[j].prn = -1;
      }
    }
    for (auto &i : bias_GAL) {
      i.resize(MAXPRNGAL + 1);
      for (int j = 0; j < MAXPRNGAL + 1; j++) {
        i[j].prn = -1;
      }
    }
    for (auto &i : bias_BDS) {
      i.resize(MAXPRNCMP + 1);
      for (int j = 0; j < MAXPRNCMP + 1; j++) {
        i[j].prn = -1;
      }
    }
  }
};

class WebDataRequestor {
 private:
  std::mutex ustec_mutex;
  std::mutex code_mutex;
  std::mutex phase_mutex;

  // Log for WEB data (Hardware biases, USTEC) record
  std::ofstream log_web;

  const std::string file_path_;
  std::string log_path[3]{};

  // product name of USTEC
  std::string ustec_fname{};

  // file name of code bias
  std::string code_bias_fname{};

  // file name of phase bias
  std::string phase_bias_fname{};
  pthread_t pid{};
  UsTecCorrData ustec_data;
  BiasCorrData code_bias, phase_bias;
  bool done;
  static size_t WriteToBuffer(void *ptr, size_t size, size_t nmemb,
                                void *userdata);
  static size_t WriteToCompressFile(void *buffer, size_t size, size_t nmemb,
                                       void *stream);
  bool DownloadFile(const char *url, const char *file_path);
  bool ReadWebpage(const char *url, std::string &buffer);
  static void ClearInputStream(std::stringstream &ss, std::string &line);
  bool RequestUsTecData();

  static void ReadGpsBiasCorr(int sv_prn, double value, const std::string &chn_type,
                              BiasCorrData &bias);
  static void ReadGalBiasCorr(int sv_prn, double value, const std::string &chn_type,
                              BiasCorrData &bias);
  static void ReadBdsBiasCorr(int sv_prn, double value, const std::string &chn_type,
                              BiasCorrData &cbias);
  static std::optional<BiasCorrData> ParseBiasFromBuff(
      const std::vector<char> &buffer, const std::string &val_title);
  bool RequestCodeBias();
  bool RequestPhaseBias();
  static void *RequestWebWrapper(void *arg);
  void RequestWebData();

 public:
  bool ready;
  // No default constructor
  WebDataRequestor() = delete;
  // constructor
  WebDataRequestor(std::string log_file_path)
      : file_path_(std::move(log_file_path)) {}
  ~WebDataRequestor() { log_web.close(); }
  // Non-copyable
  WebDataRequestor(const WebDataRequestor &) = delete;
  WebDataRequestor &operator=(const WebDataRequestor &) = delete;
  // Non-moveable
  WebDataRequestor(WebDataRequestor &&) = delete;
  WebDataRequestor &operator=(WebDataRequestor &&) = delete;

  UsTecCorrData get_ustec_data();
  BiasCorrData get_code_bias();
  BiasCorrData get_phase_bias();

  void StartRequest();
  void EndRequest();
};

#endif// VN_DGNSS_SERVER_WEB_DATA_REQUESTOR_H
