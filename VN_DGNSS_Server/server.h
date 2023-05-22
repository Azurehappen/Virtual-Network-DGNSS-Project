#pragma once
#include <netinet/tcp.h>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <iomanip>
#include "epoch_generation_helper.h"
#include "iggtrop_correction_model.h"

#define ONE_SEC_PERIOD 1000000  // 1s
#define SEND_PERIOD 5000000  // Period of server send RTCM: 5s
#define MAX_NUM_OF_CLIENTS 128    // Max no. of clients
#define BUFF_SIZE 1200       // client position buffer size
#define POSITION_MSG_HEADER "$POSECEF"

// define SockInfo struct
typedef struct SockInfo {
  SockRTCM client_sock{};
  pthread_t tid{};
  BkgDataRequestor *foo_bkg{};
  WebDataRequestor *foo_web{};
  timeperiodic::PeriodicInfoT periodic{};
  IggtropExperimentModel TropData;
} SockInfo;

bool parse_position(std::string const &message,
                    std::vector<double> &position,
                    GnssSystemInfo &infor)
// Parse a position message into a position vector. Returns true if
// parsing is successful. An example position message:
// $POSECEF 1564964.4988 4889464.1566 156489.65464 01 01 01 \r\n
{
  std::stringstream ss(message);
  std::string tmp, sys_code;
  ss >> tmp;
  if (!ss.good() || tmp != POSITION_MSG_HEADER) return false;
  std::vector<double> ret(3, 0);
  for (int i = 0; i < 3; ++i) {
    ss >> ret[i];
  }
  int disable = 0;
  for (int i = 0; i < 3; ++i) {
    int code;
    ss >> code;
    if (code>0) {
      infor.code_F1[i] = code;
    } else if (code == 0) {
      disable++;
      infor.sys[i] = false;
      infor.code_F1[i] = 0;
    }
  }
  if (!ss.good() || disable >=3 ) {
    return false;
  }
  infor.code_F2[0] = VN_CODE_GPS_C2L;
  infor.code_F2[1] = VN_CODE_GAL_C7Q;
  infor.code_F2[2] = VN_CODE_BDS_C7;
  position = ret;
  return true;
}

bool init_read_pos_client(SockInfo *client_info,
                          std::vector<double> &client_pos_ecef,
                          GnssSystemInfo &infor, char *client_ip) {
  // Read position from client
  int count = 0;
  char buff[BUFF_SIZE] = {0};
  // memset(buff, 0, sizeof(buff));
  while (count < 5) {
    int ret = recv(client_info->client_sock.fd, buff, BUFF_SIZE, 0x0000);
    if (ret == -1) {
      if (count == 4) {
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " initial receive failed by" << strerror(errno) << std::endl;
      }
    } else if (ret == 0)  // -> orderly client disconnect
    {
      *client_info->client_sock.log
          << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
          << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
          << " disconnected" << std::endl;
      return false;
    } else if (ret > 0) {
      std::string message = buff;
      if (!parse_position(message, client_pos_ecef,infor))
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " failure parsing initial client position message" << std::endl;
      else {
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "Get client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " position (ECEF): " << std::setprecision(12) << client_pos_ecef[0]
            << " " << client_pos_ecef[1] << " " << client_pos_ecef[2] << std::endl;
        break;
      }
    }
    ++count;
  }
  if (count == 5) {
    *client_info->client_sock.log
        << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
        << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
        << " initial receive failure" << std::endl;
    return false;
  } else {
    return true;
  }
}

void destruct(SockInfo *client_info)
// releases resources associated with a client thread
{
  // shutdown(client_info->fd, SHUT_RDWR);
  close(client_info->client_sock.fd);
  close(client_info->periodic.timer_fd);
  client_info->client_sock.fd = -1;
}

// thread handler function
void *pth_handler(void *arg) {
  // SockInfo *client_info = (SockInfo *)arg;
  // Use auto when initializing with a cast to avoid duplicating the type name
  auto *client_info = (SockInfo *)arg;
  int ret;
  uint16_t Port_num = ntohs(client_info->client_sock.addr.sin_port);
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_info->client_sock.addr.sin_addr), client_ip,
            INET_ADDRSTRLEN);
  // print IP and port of client
  *client_info->client_sock.log
      << vntimefunc::GetLocalTimeString() << "accept client IP: " << client_ip
      << " Port: " << Port_num
      << std::endl;
  std::vector<double> client_pos_ecef(3, 0);
  // Read position for client
  GnssSystemInfo infor;
  infor.sys.resize(3,true);
  if (!init_read_pos_client(client_info, client_pos_ecef, infor, client_ip)) {
//    close(client_info->client_sock.fd);
//    pthread_exit(nullptr);
    client_pos_ecef[0] = -2455314.231;
    client_pos_ecef[1] = -4691596.883;
    client_pos_ecef[2] = 3543996.389;
    infor.code_F1[0] = VN_CODE_GPS_C1C;
    infor.code_F2[0] = VN_CODE_GPS_C2L;
    infor.code_F1[1] = VN_CODE_GAL_C1C;
    infor.code_F2[1] = VN_CODE_GAL_C7Q;
    infor.code_F1[2] = VN_CODE_BDS_C2I;
    infor.code_F2[2] = VN_CODE_BDS_C7;
    *client_info->client_sock.log
        << vntimefunc::GetLocalTimeString() << "Use server position for client IP: " << client_ip
        << " Port: " << Port_num
        << std::endl;
  }
  std::string rst_str = "../Log/client_" +std::string(client_ip)
                        + ":" + std::to_string(Port_num) + "_log.txt";
  std::ofstream rst(rst_str.c_str());
  int iter = 0;
  double srtt, endt;

  // transfer
  while (true) {
    if (iter == 86400) { //reset the log file every 24 hours to protect storage
      iter = 0;
      rst.close();
      remove(rst_str.c_str());
      rst.open(rst_str.c_str());
    }
    // Generate RTCM data and send to client
    iter++;
    if (iter%60 == 1) { // record log by every 1 minute
      rst << "Running idx: " << iter << std::endl;
    }
    EpochGenerationHelper genRTCM(client_pos_ecef);
    client_info->client_sock.send_check = true; // If send error, false in SendRtcmMsgToClient
    srtt = vntimefunc::GetSystemTimeInSec();
    if (genRTCM.ConstructGnssMeas(client_info->foo_bkg,
                                  client_info->foo_web, rst,
                                  infor, client_info->TropData, iter)) {
      genRTCM.SendRtcmMsgToClient(&client_info->client_sock);
    }
    endt = vntimefunc::GetSystemTimeInSec();
    if (client_info->client_sock.send_check){
      if ((endt - srtt) < ONE_SEC_PERIOD)
        timeperiodic::WaitPeriod(&client_info->periodic);
      else {
        rst << "Warning: Request and Computation time exceed 1s, continue."
            << std::endl;
      }
    } else {
      break;
    }

    // Check if client update its position
    // TODO: Make a function for the entire usage below
    char buff[BUFF_SIZE] = {0};
    ret = recv(client_info->client_sock.fd, buff, sizeof(buff), MSG_DONTWAIT);
    if (ret == -1) {
      if (!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "Close client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " due to unexpected read error: " << strerror(errno)
            << std::endl;
        break;
      }
    } else if (ret == 0) {
      *client_info->client_sock.log
          << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
          << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
          << " disconnected" << std::endl;
      break;
    } else if (ret > 0) {
      *client_info->client_sock.log
          << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
          << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
          << " Position data received" << std::endl;
      std::string message = buff;
      // updates client_position
      if (!parse_position(message, client_pos_ecef, infor)) {
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " failure parsing position message" << std::endl;
      } else {
        *client_info->client_sock.log
            << vntimefunc::GetLocalTimeString() << "Get client IP: " << client_ip
            << ", Port: " << ntohs(client_info->client_sock.addr.sin_port)
            << " position (ECEF): " << std::setprecision(12) << client_pos_ecef[0]
            << " " << client_pos_ecef[1] << " " << client_pos_ecef[2]
            << std::endl;
      }
    }
  }
  rst.close();
  close(client_info->client_sock.fd);
  close(client_info->periodic.timer_fd);
  client_info->client_sock.fd = -1;
  pthread_exit(nullptr);
}
