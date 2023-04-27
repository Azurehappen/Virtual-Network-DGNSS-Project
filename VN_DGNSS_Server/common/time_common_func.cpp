#include "time_common_func.h"

namespace timeperiodic {
int MakePeriodic(unsigned int period_micro_second, PeriodicInfoT &info) {
  int ret;
  unsigned int ns;
  unsigned int sec;
  int fd;
  struct itimerspec itval {};

  /* Create the timer */
  fd = timerfd_create(CLOCK_MONOTONIC, 0);
  info.wakeups_missed = 0;
  info.timer_fd = fd;
  if (fd == -1) {
    return fd;
  }
  /* Make the timer periodic */
  sec = period_micro_second / 1000000;
  ns = (period_micro_second - (sec * 1000000)) * 1000;
  itval.it_interval.tv_sec = sec;
  itval.it_interval.tv_nsec = ns;
  itval.it_value.tv_sec = sec;
  itval.it_value.tv_nsec = ns;
  ret = timerfd_settime(fd, 0, &itval, nullptr);
  return ret;
}

void WaitPeriod(PeriodicInfoT *info) {
  unsigned long long missed;
  size_t ret;

  /* Wait for the next timer event. If we have missed any the
number is written to "missed" */
  ret = read(info->timer_fd, &missed, sizeof(missed));
  if (ret == -1) {
    perror("read timer");
    return;
  }

  info->wakeups_missed += missed;
}
}  // namespace timeperiodic

namespace vntimefunc {
void DateToTimeOfWeek(std::vector<double> date_time, int &gps_week,
                      int &gps_dow, double &gps_sow) {
  const int doy[] = {1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
  const int gps_start_num =
      3657; /* gps time reference [1980 1 6], date number start at [1970 1 1]*/
  int days, delta_day;
  int year = (int)date_time[0], mon = (int)date_time[1],
      day = (int)date_time[2];

  if (year < 1970 || 2099 < year || mon < 1 || 12 < mon) return;

  /* leap year if year%4==0 in 1901-2099 */
  days = (year - 1970) * 365 + (year - 1969) / 4 + doy[mon - 1] + day - 2 +
         (year % 4 == 0 && mon >= 3 ? 1 : 0);
  delta_day = days - gps_start_num;
  gps_week = floor(static_cast<double>(delta_day) / 7);
  gps_dow = delta_day - gps_week * 7;
  gps_sow =
      gps_dow * 86400 + date_time[3] * 3600 + date_time[4] * 60 + date_time[5];
}

double LimitGpsTime(double time_diff) {
  if (time_diff > 302400) {
    time_diff -= 604800;
  } else if (time_diff < -302400) {
    time_diff += 604800;
  }
  return time_diff;
}

double GetSystemTimeInSec() {
  struct timezone tz({0, 0});
  timeval tv{};
  gettimeofday(&tv, &tz);
  return (double)tv.tv_sec * 1000000 + tv.tv_usec;
}

void GetGpsTimeNow(std::vector<double> &date_time_gps, int &doy,
                   gtime_t &gpst_now) {
  time_t tt;
  struct tm *pt;
  time(&tt);
  pt = gmtime(&tt);
  // cout<<"Current UTC time (hour:minute:sec): "<<pt->tm_hour<<"
  // "<<pt->tm_min<<" "<<pt->tm_sec<<endl;
  // rst << "Current UTC time (hour:minute:sec): " << pt->tm_hour << " "
  //   << pt->tm_min << " " << pt->tm_sec << endl;
  time_t rawtime;
  struct tm *ptm;
  time(&rawtime);
  rawtime += 18;
  ptm = gmtime(&rawtime);
  date_time_gps[0] = ptm->tm_year + 1900;
  date_time_gps[1] = ptm->tm_mon + 1;
  date_time_gps[2] = ptm->tm_mday;
  date_time_gps[3] = ptm->tm_hour;
  date_time_gps[4] = ptm->tm_min;
  date_time_gps[5] = ptm->tm_sec;
  doy = ptm->tm_yday;
  double epoch_gpst[6];
  for (int i = 0; i < 6; i++) {
    epoch_gpst[i] = date_time_gps[i];
  }
  gpst_now = epoch2time(epoch_gpst);
}

// Get local time string
std::string GetLocalTimeString() {
  time_t rawtime;
  struct tm *ltm;
  time(&rawtime);
  ltm = localtime(&rawtime);
  char buf[28];
  sprintf(buf, "[%04d-%02d-%02d %02d:%02d:%02d]: ", ltm->tm_year + 1900,
          ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
          ltm->tm_sec);
  return {buf};
}

// Get time string for file
std::string GetLocalTimeStringForLog() {
  time_t rawtime;
  struct tm *ltm;
  time(&rawtime);
  ltm = localtime(&rawtime);
  char buf[9];
  sprintf(buf, "%04d%02d%02d", ltm->tm_year + 1900, ltm->tm_mon + 1,
          ltm->tm_mday);
  return {buf};
}

int GetCurrentYear() {
  time_t rawtime;
  struct tm *ptm;
  time(&rawtime);
  ptm = gmtime(&rawtime);
  return ptm->tm_year + 1900;
}

uint64_t GetSecFromTimeval(timeval tv) {
  return ((double)tv.tv_sec) + ((double)tv.tv_usec) / 1000000.;
}
}  // namespace vntimefunc