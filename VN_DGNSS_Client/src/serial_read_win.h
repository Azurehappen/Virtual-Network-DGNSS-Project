#ifdef WIN32
#ifndef WADGNSS_CLIENT_SERIAL_READ_WIN_H
#define WADGNSS_CLIENT_SERIAL_READ_WIN_H
#pragma once
#include "utility.h"
#define POS_BUFSIZE       10000
#ifdef WIN32
#define READ_INTV         500  // 0.5s
#else
#define READ_INTV         500000  // 0.5s
#endif
class serial_read {
private:
  std::mutex pos_mutex;
  std::vector<double> LLA_pos;
  const std::string POSLOG_PATH;
  std::ofstream poslog;
  const std::string com_port;
  HANDLE com_fd;
  HANDLE hThread;
  static std::string get_postfix();
  static DWORD WINAPI reading_wrapper(void *arg);
  void posdata_reading();
public:
  serial_read() = delete;
 serial_read(const HANDLE com_fd, const std::string &com_port,
              std::string pos_log_path)
      : com_port(com_port),
        com_fd(com_fd), POSLOG_PATH(std::move(pos_log_path)),
        LLA_pos(3, 0){};
  ~serial_read() = default;
  // Non-copyable
  serial_read(const serial_read &) = delete;
  serial_read &operator=(const serial_read &) = delete;
  // Non-moveable
  serial_read(serial_read &&) = delete;
  serial_read &operator=(serial_read &&) = delete;
  bool pos_ready;
  bool start_err;
  std::vector<double> get_LLA_pos();
  void start_serial_read(std::ofstream &runlog);
  void end_serial_read();
};
#endif//WADGNSS_CLIENT_SERIAL_READ_WIN_H
#endif