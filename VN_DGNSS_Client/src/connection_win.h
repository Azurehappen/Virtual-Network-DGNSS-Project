#ifdef WIN32
#ifndef WADGNSS_CLIENT_CONNECTION_WIN_H
#define WADGNSS_CLIENT_CONNECTION_WIN_H
#pragma once
#include "utility.h"
#pragma comment(lib, "Ws2_32.lib")
//#define SERVER_IP "192.168.1.118"
//#define SERVER_PORT 3636
#define RTCM_BUFSIZE      1200
#ifdef WIN32
#define READ_PERIOD 600          // read period of rover position in seconds
#define UBX_wait 100             // 0.1s
#define LOOP_WAIT 1           // 0.001s
#define INTUBX_INTERVAL 1000        // 1s
#else
#define READ_PERIOD       600  // read period of rover position in seconds
#define UBX_wait 100000     // 0.1s
#define LOOP_WAIT         1000   // 0.001s
#define INTUBX_INTERVAL   1 // 1s
#endif
bool opencom_port(const std::string &com_port, HANDLE &fd);
SOCKET get_socket_fd(const char *SERVER_IP, int SERVER_PORT,
                  std::ofstream &runlog);
bool interpret_GNS(std::vector<double> &LLA, const std::string &GNSstr);
std::string msg_to_server(std::vector<double> LLA, const std::string& sys_set);
void close_all(HANDLE rover_fd, SOCKET client_fd, std::ofstream &runlog);
bool init_ublox(HANDLE fd, int UBX_TYPE);
#endif  // WADGNSS_CLIENT_CONNECTION_WIN_H
#endif