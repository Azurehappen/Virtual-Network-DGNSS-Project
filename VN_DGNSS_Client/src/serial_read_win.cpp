#ifdef WIN32
#include "serial_read_win.h"

// Get days stamp for ROVERLOG: YYYYMMDD
std::string serial_read::get_postfix() {
  time_t rawtime;
  struct tm ltm;
  time(&rawtime);
  localtime_s(&ltm, & rawtime);
  char buf[9];
  sprintf_s(buf, "%04d%02d%02d", ltm.tm_year + 1900,
          ltm.tm_mon + 1, ltm.tm_mday);
  return std::string(buf);
}

// Read data from rover
void serial_read::posdata_reading() {
  while (true) {
    char pos_buff[POS_BUFSIZE] = {0};
    DWORD ns;
    bool ret = ReadFile(com_fd, pos_buff, POS_BUFSIZE, &ns, NULL);
    if (!ret) {
      poslog << local_tstr()
             << "Read rover failed: " << GetLastError()
             << std::endl;
      CloseHandle(hThread);
    } else {
      std::stringstream out_ss(pos_buff);
      std::string line;
      while (getline(out_ss, line)) {
        if (line.size() > 30) {
          // write GxGNS message to log file
          if ((line.substr(0, 2) + line.substr(3, 3)) == "$GGNS") {
            poslog << line << std::endl;
            std::vector<double> get_LLA(3,0);
            if (interpret_GNS(get_LLA, line)) {
              std::lock_guard<std::mutex> lock(pos_mutex);
              for (int i = 0; i<3; i++) {LLA_pos[i]=get_LLA[i];}
              pos_ready = true;
            }
          }
          // report PUBX 00 message if it's available
          if (line.substr(0, 8) == "$PUBX,00") {
            poslog << line << std::endl;
          }
        }
      }
    }
    Sleep(READ_INTV);
  }
}

std::vector<double> serial_read::get_LLA_pos() {
  return LLA_pos;
}

// Wrapper for serial reading function
DWORD WINAPI serial_read::reading_wrapper(void *arg) {
  reinterpret_cast<serial_read *>(arg)->posdata_reading();
  return NULL;
}

void serial_read::start_serial_read(std::ofstream &runlog) {
  std::string FILE_NAME = POSLOG_PATH + "ROVERLOG_"
                          + get_postfix() + ".log";
  poslog.open(FILE_NAME);
  start_err = false;
  pos_ready = false;
  if (!poslog.is_open()) {
    runlog<< local_tstr() <<"rover log file cannot be open"<<std::endl;
    start_err = true;
  }
  //com_fd = CreateFile(com_port.c_str(), 
  //                    GENERIC_READ | GENERIC_WRITE, 
  //                    0,      
  //                    NULL,                   // No Security
  //                    OPEN_EXISTING,          // Open existing port only
  //                    FILE_ATTRIBUTE_NORMAL,  // Non Overlapped I/O
  //                    NULL);
  //if (com_fd == INVALID_HANDLE_VALUE) {
  //  runlog << local_tstr() << "com port cannot be open by read only, errno:"
  //         << GetLastError() << std::endl;
  //  start_err = true;
  //}
  if (!start_err) {
    hThread = CreateThread(NULL, 0, reading_wrapper, this, 0, NULL);
  }
}

void serial_read::end_serial_read() {
  poslog.close();
  CloseHandle(com_fd);
  CloseHandle(hThread);
}
#endif