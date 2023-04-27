#pragma once

#include "rtklib.h"
#include "time_common_func.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

// define Sock for CreateRtcmMsg struct
struct SockRTCM{
  int fd;
  struct sockaddr_in addr;
  std::ostream *log; // Server log
  bool send_check; // Check if send data success
  std::ostream *rtcm_log;
};

// Create RTCM message (modified function from RTKLIB)
int CreateRtcmMsg(int n, const int *type, int m, SockRTCM *client_info,
              std::vector<double> sta_pos, std::vector<obsd_t> data_obs);

