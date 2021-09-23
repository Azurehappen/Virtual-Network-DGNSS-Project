
#ifndef WADGNSS_SERVER_REQUESTOR_WEB_H
#define WADGNSS_SERVER_REQUESTOR_WEB_H
#pragma once
#include <arpa/inet.h>
#include <curl/curl.h>
#include "timecmn.h"
#include <zlib.h>
#include "rtklib.h"
#include <mutex>
#include <sstream>
#include <fstream>
#include <iostream>

#define code_GPS_NONE   0
#define code_GPS_C1C    1
#define code_GPS_C1W    2
#define code_GPS_C2C    3
#define code_GPS_C2W    4
#define code_GPS_C2L    5
#define MAX_CODE_GPS    5

#define code_GAL_NONE   0
#define code_GAL_C1C    1
#define code_GAL_C1X    2
#define code_GAL_C6C    3
#define code_GAL_C5Q    4
#define code_GAL_C5X    5
#define code_GAL_C7Q    6
#define code_GAL_C7X    7
#define MAX_CODE_GAL    7

#define code_BDS_NONE   0
#define code_BDS_C2I    1
#define code_BDS_C6I    2
#define code_BDS_C7     3
#define MAX_CODE_BDS    3

#define MAX_CODE_ELE    10

static constexpr const char *USTEC_URL =
    "https://services.swpc.noaa.gov/text/"
    "us-tec-total-electron-content.txt";  // URL of USTEC data
static constexpr int USTEC_NUM_ROW =
    52;                              // number of lines needed in USTEC data
static constexpr const char *CODE_BIAS_URL_header =
    "ftp://ftp.gipp.org.cn/product/dcb/daybias/";  // URL of CODE_BIAS header
static constexpr const char *PHASE_BIAS_URL_header =
    "ftp://igs.gnsswhu.cn/pub/whu/phasebias/";  // URL of PHASE_BIAS header
static constexpr int USTEC_PERIOD =
    60;                               // update period of USTEC data in seconds
static constexpr int TIMEOUT = 60;  // timeout parameter for curl function
static constexpr int HW_PERIOD =
    86400;  // update period of hardware bias in seconds
static constexpr int CHECK_USTEC_PERIOD =
    60*1000000;  // check USTEC period in microseconds

// Struct for download ftp file
struct FtpFile
{
  const char *filepath;
  FILE *fp;
};

struct ustec {
  double time[6]{};
  std::vector<std::vector<int>> data;
};

struct bias_element {
  int prn; // when not available, this value is -1
  double value;
};

struct code_bias_strc {
  std::vector<bias_element> cbias_GPS[MAX_CODE_GPS];
  std::vector<bias_element> cbias_GAL[MAX_CODE_GAL];
  std::vector<bias_element> cbias_BDS[MAX_CODE_BDS];
  code_bias_strc() {
    for (auto & i : cbias_GPS) {
      i.resize(MAXPRNGPS+1);
      for (int j = 0;j<MAXPRNGPS+1;j++) {
        i[j].prn = -1;
      }
    }
    for (auto & i : cbias_GAL) {
      i.resize(MAXPRNGAL+1);
      for (int j = 0;j<MAXPRNGAL+1;j++) {
        i[j].prn = -1;
      }
    }
    for (auto & i : cbias_BDS) {
      i.resize(MAXPRNCMP+1);
      for (int j = 0;j<MAXPRNCMP+1;j++) {
        i[j].prn = -1;
      }
    }
  }
};

class requestor_web {
private:
  std::mutex ustec_mutex;
  std::mutex code_mutex;
  std::ofstream log_WEB; // Log for WEB data (Hardware biases, USTEC) record
  const std::string FILE_PATH;
  std::string log_path[3]{};
  std::string ustec_fname{}; // product name of USTEC
  std::string code_bias_fname{}; // file name of code bias
  pthread_t pid{};
  ustec ustec_data;
  code_bias_strc code_bias;
  bool done;
  static size_t write_to_buffer(void *ptr, size_t size, size_t nmemb,
                                void *userdata);
  static size_t write_to_compress_file(void *buffer, size_t size,
                                       size_t nmemb, void *stream);
  bool download_file(const char *url, const char *file_path);
  bool read_webpage(const char *url, std::string &buffer);
  static void clear_input_stream(std::stringstream &ss, std::string &line);
  bool request_ustec_data();

  static void read_GPS_cbias(int sv_prn,double value, const std::string& TYPE,
                             code_bias_strc &new_code_bias);
  static void read_GAL_cbias(int sv_prn,double value, const std::string& TYPE,
                             code_bias_strc &new_code_bias);
  static void read_BDS_cbias(int sv_prn,double value, const std::string& TYPE,
                             code_bias_strc &new_code_bias);
  bool parse_code_bias(const std::string& buffer);
  bool request_code_bias();
  static void *request_web_wrapper(void *arg);
  void request_web_data();

public:
  bool ready;
  // No default constructor
  requestor_web() = delete;
  // constructor
  requestor_web(std::string log_FILE_PATH)
      : FILE_PATH(std::move(log_FILE_PATH)) {}
  ~requestor_web() { log_WEB.close(); }
  // Non-copyable
  requestor_web(const requestor_web &) = delete;
  requestor_web &operator=(const requestor_web &) = delete;
  // Non-moveable
  requestor_web(requestor_web &&) = delete;
  requestor_web &operator=(requestor_web &&) = delete;

  ustec get_ustec_data();
  code_bias_strc get_code_bias();

  void start_request();
  void end_request();
};

#endif // WADGNSS_SERVER_REQUESTOR_WEB_H
