#include "connection.h"
#include "serial_read.h"
int main(int argc, char* argv[]) {
  // input check
  if (argc < 6) {
    // Rover_type: 0 for others, 1 for M8P, 2 for ZED-F9P
    std::cerr << "Not enough arguements.\n";
    std::cerr << "eg: sudo ./WADGNSS_Client COM_PORT "
                 "Rover_type Server_host Server_port 010101"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  const std::string com_port = argv[1];
  const std::string UBX_TYPE_ss = argv[2];
  int UBX_TYPE = stoi(UBX_TYPE_ss);
  const char* SERVER_ADDR = argv[3];
  const std::string SERVER_PORT_ss = argv[4];
  int SERVER_PORT = stoi(SERVER_PORT_ss);
  const std::string sys_selection = argv[5];
  std::ofstream runlog("running_log.log");
  if (sys_selection.size()>6) {
    runlog << "unusual system selection. Sever will not responds" << std::endl;
  }
  // 1. Build connection with rover
  int rover_fd;
  runlog << local_tstr() << "Start connecting receiver." << std::endl;
  if (!opencom_port(com_port, rover_fd)) {
    runlog << local_tstr() << "COM port open error: " << com_port
           << ", pls check com number or permission" << std::endl;
    runlog.close();
    exit(EXIT_FAILURE);
  }
  runlog << local_tstr() << "Succeed." << std::endl;
  if (UBX_TYPE == 0) {
    runlog << local_tstr() << "Receiver configured by user." << std::endl;
  } else {
    if (!init_ublox(rover_fd, UBX_TYPE)) {
      runlog << local_tstr()
             << "Initialize u-blox failed. please check your input info"
             << std::endl;
      runlog.close();
      close(rover_fd);
      exit(EXIT_FAILURE);
    }
    runlog << local_tstr() << "Initialize u-blox type: " << UBX_TYPE
           << " (1 for M8P, 2 for ZED-F9P)." << std::endl;
  }
  // 2. Build connection with VN-DGNSS server
  int client_fd = get_socket_fd(SERVER_ADDR, SERVER_PORT, runlog);
  if (client_fd == -1) {
    runlog << local_tstr() << "Build connection with server failed"
           << std::endl;
    runlog.close();
    close(rover_fd);
    exit(EXIT_FAILURE);
  }
  runlog << local_tstr() << "Build connection with server succeed."
         << std::endl;
  int count = 1;
  std::string poslog("./");
  runlog << local_tstr() << "Start read pos info (NMEA $GxGNS) from rover."
         << std::endl;
  serial_read* foo;
  // Another thread for reading serial port
  foo = new serial_read(com_port, poslog);
  foo->start_serial_read(runlog);
  if (foo->start_err) {
    close_all(rover_fd, client_fd, runlog);
    exit(EXIT_FAILURE);
  }
  while (true) {
    if (foo->pos_ready) {
      std::string pos_ecef = msg_to_server(foo->get_LLA_pos(),sys_selection);
      char pos_buff[RTCM_BUFSIZE];
      strcpy(pos_buff, pos_ecef.c_str());
      int send_ret = send(client_fd, pos_buff, RTCM_BUFSIZE, 0);
      if (send_ret == -1) {
        runlog << local_tstr()
               << "Send $POSECEF to server failed: " << strerror(errno)
               << std::endl;
      } else {
        runlog << local_tstr() << "Send to server: " << pos_ecef << std::endl;
      }
      break;
    }
    // Initially read receiver 2 minutes avoid rover in warm up
    sleep(INTUBX_INTERVAL);
    count++;
    if (count > 120) {
      runlog << local_tstr()
             << "Can't get position info from rover, "
                "please check if the NMEA protocol GNS massage is enabled."
             << std::endl;
      close_all(rover_fd, client_fd, runlog);
      foo->end_serial_read();
      exit(EXIT_FAILURE);
    }
  }
  timeval tv{};
  struct timezone tz({0, 0});
  gettimeofday(&tv, &tz);
  uint64_t read_t = get_sec(tv);
  while (true) {
    gettimeofday(&tv, &tz);
    uint64_t now = get_sec(tv);
    // Send position to server every 10 minutes
    if (now >= read_t + READ_PERIOD) {
      read_t = now;
      std::string pos_ecef = msg_to_server(foo->get_LLA_pos(),sys_selection);
      char pos_buff[RTCM_BUFSIZE];
      strcpy(pos_buff, pos_ecef.c_str());
      int send_ret = send(client_fd, pos_buff, RTCM_BUFSIZE, 0);
      if (send_ret == -1) {
        runlog << local_tstr()
               << "Send $POSECEF to server failed: " << strerror(errno)
               << std::endl;
      } else {
        runlog << local_tstr() << "Send to server: " << pos_ecef << std::endl;
      }
    }
    // Read data from server
    char RTCM_Buffer[RTCM_BUFSIZE] = {0};
    int read_ret = recv(client_fd, RTCM_Buffer, sizeof(RTCM_Buffer), 0);
    if (read_ret < 0) {
      if (!(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)) {
        runlog << local_tstr() << "recv server data error: " << strerror(errno)
               << std::endl;
        break;
      }
    } else if (read_ret == 0) {
      runlog << local_tstr() << "Server has closed connection." << std::endl;
      break;
    } else if (read_ret > 0) {
      if (write(rover_fd, RTCM_Buffer, sizeof(RTCM_Buffer)) == -1) {
        runlog << local_tstr() << "Send to rover failed: " << strerror(errno)
               << std::endl;
        break;
      }
    }
    usleep(LOOP_WAIT);
  }
  close_all(rover_fd, client_fd, runlog);
  foo->end_serial_read();
  exit(EXIT_FAILURE);
}
