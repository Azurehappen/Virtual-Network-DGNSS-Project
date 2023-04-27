#pragma once
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include "rtklib.h"

// Periodic time functions
namespace timeperiodic {
struct PeriodicInfoT {
  int timer_fd;
  unsigned long long wakeups_missed;
};

int MakePeriodic(unsigned int period_micro_second, PeriodicInfoT &info);
void WaitPeriod(PeriodicInfoT *info);
}  // namespace timeperiodic

// VN-DGNSS project common time functions
namespace vntimefunc {
void DateToTimeOfWeek(std::vector<double> date_time, int &gps_week,
                      int &gps_dow, double &gps_sow);
double LimitGpsTime(double time_diff);
double GetSystemTimeInSec();
void GetGpsTimeNow(std::vector<double> &date_time_gps, int &doy,
                   gtime_t &gpst_now);
std::string GetLocalTimeString();
std::string GetLocalTimeStringForLog();
int GetCurrentYear();
uint64_t GetSecFromTimeval(timeval tv);
}  // namespace vntimefunc
