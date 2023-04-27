#pragma once
#include <sys/time.h>
#include <sys/timerfd.h>
#include "rtklib.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
struct periodic_info_t {
  int timer_fd;
  unsigned long long wakeups_missed;
};

int make_periodic(unsigned int period_micro_second, periodic_info_t &info);
void wait_period(periodic_info_t *info);
void datetotow(std::vector<double> date_time, int& gps_week, int& gps_dow, double& gps_sow);
double limit_gpst(double time_diff);
double get_time_sec();
void  get_gpst_now(std::vector<double> &date_time_gps,int &doy,gtime_t &gpstnow);
std::string get_time();
std::string get_time_log();
int get_year();
uint64_t get_sec(timeval tv);
