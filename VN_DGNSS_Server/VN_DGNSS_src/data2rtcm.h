#pragma once

#include <math.h>
#include <sys/socket.h>
#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <vector>
#include "rtklib.h"
#include "timecmn.h"

// define Sock for data2rtcm struct
struct SockRTCM{
  int fd;
  struct sockaddr_in addr;
  std::ostream *log; // Server log
  bool send_check; // Check if send data success
  std::ostream *rtcm_log;
};

int data2rtcm(int n, const int *type, int m, SockRTCM *client_info,
              std::vector<double> sta_pos, std::vector<obsd_t> data_obs);

