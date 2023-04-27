#include "server.h"

int main(int argc, char *argv[]) {
  // check user input port
  if (argc < 3) {
    std::cerr << "eg: ./server IP Port" << std::endl;
    exit(EXIT_FAILURE);
  }
  char const *IPaddr = argv[1];            // user input server IP
  uint16_t const port_nu = std::stoi(argv[2]);  // user input server port number
  std::ofstream serverlog;
  serverlog.open("../Log/serverlog.txt",std::ios::app); // open file by append
  // 1.create a socket
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    std::cerr << vntimefunc::GetLocalTimeString() << "err: create socket fail! caused by "
              << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  // 1.1 Allow to reuse the same port if socket closed or breakdown
  int bReuse = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &bReuse,
                 sizeof(bReuse)) == -1) {
    std::cerr << vntimefunc::GetLocalTimeString() << "err: set socket REUSE fail! caused by "
              << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  //  bool noLinger = false;
  //  setsockopt(socket_fd,SOL_SOCKET,SO_DONTLINGER,(const
  //  char*)&noLinger,sizeof(bool));
  // 1.2 Turn off Nagle
  int ON = 1;
  if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&ON,
                 sizeof(ON)) == -1) {
    std::cerr << vntimefunc::GetLocalTimeString() << "err: set socket TCP_NODELAY fail! caused by "
              << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  // 2.bind ip and port
  struct sockaddr_in server_addr {};
  bzero((char *)&server_addr, sizeof(server_addr));

  server_addr.sin_family = AF_INET;                     // addr family
  inet_pton(AF_INET, IPaddr, &(server_addr.sin_addr));  // ip addr
  // server_addr.sin_addr.s_addr = inet_addr(IPaddr); // ip addr
  server_addr.sin_port = htons(port_nu);  // port num

  int bind_ret =
      bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (bind_ret < 0) {
    std::cerr << vntimefunc::GetLocalTimeString() << "err: bind fail! caused by " << strerror(errno)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // 3.listen
  int listen_ret = listen(socket_fd, MAX_NUM_OF_CLIENTS);
  if (listen_ret == -1) {
    std::cerr << vntimefunc::GetLocalTimeString() << "err: listen fail! caused by " << strerror(errno)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  serverlog << vntimefunc::GetLocalTimeString() << "The server started..." << std::endl;
  serverlog << vntimefunc::GetLocalTimeString() << "Listen on port: " << port_nu << std::endl;
  serverlog << vntimefunc::GetLocalTimeString() << "Waiting for client..." << std::endl;

  // 4.Start requestor
  BkgDataRequestor *foo_bkg;
  WebDataRequestor *foo_web;
  std::string FOLDER_PATH = "../Log/";  // Specify the path of correction data
  foo_bkg = new BkgDataRequestor(FOLDER_PATH);
  foo_web = new WebDataRequestor(FOLDER_PATH);
  // start requesting data
  foo_bkg->StartRequestor();
  foo_web->StartRequest();
  while (!(foo_web->ready && foo_bkg->ssr_ready && foo_bkg->eph_ready)) {
    sleep(2);
  }
  // Get empirical Trop model data
  IggtropExperimentModel TropData = GetIggtropCorrDataFromFile("../vn_dgnss_source/IGGtropSHexpModel.ztd");
  // 5.wait and connect, pthread_create
  unsigned i;
  SockInfo client_info[MAX_NUM_OF_CLIENTS];  // maximum number of threads
  // set fd=-1
  for (i = 0; i < sizeof(client_info) / sizeof(client_info[0]); ++i) {
    client_info[i].client_sock.fd = -1;
  }
  socklen_t len = sizeof(struct sockaddr_in);
  std::ofstream rtcmlog;
  rtcmlog.open("../../../Log/rtcm_log.txt");
  while (true) {
    for (i = 0; i < MAX_NUM_OF_CLIENTS; i++) {
      if (client_info[i].client_sock.fd == -1) break;
    }
    if (i >= MAX_NUM_OF_CLIENTS - 1) {
      // Searching available site when clients reach maximum
      bool wait = true;
      while (wait) {
        sleep(1);
        for (int k = 0; k < MAX_NUM_OF_CLIENTS; k++) {
          if (client_info[k].client_sock.fd == -1) {
            i = k;
            wait = false;
            break;
          }
        }
      }
    }

    // main thread: wait and connection
    client_info[i].client_sock.fd = accept(
        socket_fd, (struct sockaddr *)&client_info[i].client_sock.addr, &len);
    if (client_info[i].client_sock.fd == -1) {
      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(client_info[i].client_sock.addr.sin_addr), client_ip,
                INET_ADDRSTRLEN);
      serverlog << vntimefunc::GetLocalTimeString() << "Failure accepting Client IP: " << client_ip
                << " Port: " << ntohs(client_info[i].client_sock.addr.sin_port)
                << " caused by " << strerror(errno) << std::endl;
      close(client_info[i].client_sock.fd);
      continue;
    }
    struct timeval timeout_recv = {1, 0};  // 1s
    struct timeval timeout_send = {1, 0};  // 1s
    // Set send timeout
    setsockopt(client_info[i].client_sock.fd, SOL_SOCKET, SO_SNDTIMEO,
               (const char *)&timeout_send, sizeof(timeout_send));
    // Set recv timeout
    setsockopt(client_info[i].client_sock.fd, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&timeout_recv, sizeof(timeout_recv));
    client_info[i].foo_bkg = foo_bkg;
    client_info[i].foo_web = foo_web;
    client_info[i].client_sock.log = &serverlog;
    client_info[i].client_sock.rtcm_log = &rtcmlog;
    client_info[i].TropData = TropData;
    // Make Periodic for the client
    timeperiodic::MakePeriodic(SEND_PERIOD, client_info[i].periodic);
    // create pthread to transfer
    pthread_create(&client_info[i].tid, nullptr, pth_handler, &client_info[i]);
    // detach a thread
    pthread_detach(client_info[i].tid);
  }

  // 6.close
  foo_bkg->EndRequestor();
  foo_web->EndRequest();
  close(socket_fd);

  // terminate main thread
  pthread_exit(nullptr);
}
