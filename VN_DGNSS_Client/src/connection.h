
#ifndef WADGNSS_CLIENT_CONNECTION_H
#define WADGNSS_CLIENT_CONNECTION_H
#pragma once
#include "utility.h"
//#define SERVER_IP "192.168.1.118"
//#define SERVER_PORT 3636
#define UBX_wait          100000  // 0.1s
#define RTCM_BUFSIZE      1200
#define READ_PERIOD       600  // read period of rover position in seconds
#define LOOP_WAIT         1000   // 0.001s
#define INTUBX_INTERVAL   1 // 1s
bool opencom_port(const std::string &com_port, int &fd);
int get_socket_fd(const char *SERVER_IP, int SERVER_PORT,
                  std::ofstream &runlog);
std::string msg_to_server(std::vector<double> LLA, const std::string& sys_set);
void close_all(int rover_fd, int client_fd, std::ofstream &runlog);
bool init_ublox(int fd, int UBX_TYPE);
#endif  // WADGNSS_CLIENT_CONNECTION_H
