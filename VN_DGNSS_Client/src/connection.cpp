#include "connection.h"
// Open serial port
bool opencom_port(const std::string &com_port, int &fd) {
  char const *rover = com_port.c_str();
  if ((fd = open(rover, O_WRONLY | O_NOCTTY | O_NONBLOCK)) < 0) {
    return false;
  }
  // Set USB configuration
  int bsize = 8, stopb = 1;
  const speed_t brate = B9600;
  char parity = 'N', fctr[64] = "";
  struct termios ios = {0};
  tcgetattr(fd, &ios);
  ios.c_iflag = 0;
  ios.c_oflag = 0;
  ios.c_lflag = 0;    /* non-canonical */
  ios.c_cc[VMIN] = 0; /* non-block-mode */
  ios.c_cc[VTIME] = 0;
  cfsetospeed(&ios, brate);
  cfsetispeed(&ios, brate);
  ios.c_cflag |= bsize == 7 ? CS7 : CS8;
  ios.c_cflag |=
      parity == 'O' ? (PARENB | PARODD) : (parity == 'E' ? PARENB : 0);
  ios.c_cflag |= stopb == 2 ? CSTOPB : 0;
  ios.c_cflag |= !strcmp(fctr, "rts") ? CRTSCTS : 0;
  tcsetattr(fd, TCSANOW, &ios);
  tcflush(fd, TCIOFLUSH);
  return true;
}

// String number with customized precision
static std::string to_string_with_precision(const double val, int precison) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(precison) << val;
  return out.str();
}

// Build connection with server
int get_socket_fd(const char *HOST_ADDR, int SERVER_PORT,
                  std::ofstream &runlog) {
  // 1.create a socket
  int client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client_fd == -1) {
    runlog << local_tstr() << "socket start error: " << strerror(errno)
           << std::endl;
    return -1;
  }
  // 2.connect server
  struct sockaddr_in server_addr {};
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  // server_addr.sin_addr.s_addr = htonl();
  // inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);
  struct hostent *host;
  host = gethostbyname(HOST_ADDR);
  if (host == nullptr) {
    switch (h_errno) {
      case HOST_NOT_FOUND:
        runlog << local_tstr() << "The specified host is unknown." << std::endl;
      case NO_ADDRESS:
        runlog << local_tstr()
               << "The requested name is valid but does not have an IP address."
               << std::endl;
      case NO_RECOVERY:
        runlog << local_tstr() << "A nonrecoverable name server error occurred."
               << std::endl;
      case TRY_AGAIN:
        runlog << local_tstr()
               << "A temporary error occurred on an authoritative name server. "
                  "Try again later."
               << std::endl;
    }
    return -1;
  }
  memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

  // 3.connect
  if (connect(client_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    runlog << local_tstr() << "connection start error: " << strerror(errno)
           << std::endl;
    close(client_fd);
    return -1;
  }
  // set socket
  int mode = 1;
  if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&mode,
                 sizeof(mode)) == -1) {
    runlog << local_tstr() << "setsockopt error: " << strerror(errno)
           << std::endl;
    close(client_fd);
    return -1;
  }
  struct timeval timeout_send = {2, 0};  // 2s
  struct timeval timeout_recv = {2, 0};  // 2s
  // Set send timeout
  setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout_send,
             sizeof(timeout_send));
  setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_recv,
             sizeof(timeout_send));
  return client_fd;
}

// Create string massage of ECEF position information
std::string msg_to_server(std::vector<double> LLA, const std::string& sys_set) {
  double Lat = D2R(LLA[0]);
  double Lon = D2R(LLA[1]);
  double Alt = LLA[2];
  std::vector<double> lla_pos{Lat, Lon, Alt};
  std::vector<double> ecef_p(3, 0);
  pos2ecef(lla_pos, ecef_p);
  std::string pos_ecef = "$POSECEF " + to_string_with_precision(ecef_p[0], 4) +
                         " " + to_string_with_precision(ecef_p[1], 4) + " " +
                         to_string_with_precision(ecef_p[2], 4) + " "
                         + sys_set.substr(0,2) + " "
                         + sys_set.substr(2,2) + " "
                         + sys_set.substr(4,2) + " " + "\r\n";
  return pos_ecef;
}

void close_all(int rover_fd, int client_fd, std::ofstream &runlog) {
  runlog.close();
  close(rover_fd);
  close(client_fd);
}

// Initially configure ublox
bool init_ublox(int fd, int UBX_TYPE) {
  if (UBX_TYPE == 2) {
    if (write(fd, (char *)ZEDF9P_MSGOUT_OFF, sizeof(ZEDF9P_MSGOUT_OFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)ZEDF9P_GGAGNS_ON, sizeof(ZEDF9P_GGAGNS_ON)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)ZEDF9P_15ELEV, sizeof(ZEDF9P_15ELEV)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)ZEDF9P_GPSONLY, sizeof(ZEDF9P_GPSONLY)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    return true;
  } else if (UBX_TYPE == 1) {
    if (write(fd, (char *)M8P_GGAOFF, sizeof(M8P_GGAOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_GLLOFF, sizeof(M8P_GLLOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_GSAOFF, sizeof(M8P_GSAOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_GSVOFF, sizeof(M8P_GSVOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_RMCOFF, sizeof(M8P_RMCOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_VTGOFF, sizeof(M8P_VTGOFF)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_GNSON, sizeof(M8P_GNSON)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_GPSONLY, sizeof(M8P_GPSONLY)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    if (write(fd, (char *)M8P_15ELEV, sizeof(M8P_15ELEV)) < 0) {
      return false;
    }
    usleep(UBX_wait);
    return true;
  } else {
    return false;
  }
}