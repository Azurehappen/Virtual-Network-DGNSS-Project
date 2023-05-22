
#ifndef WIN32
#include <dirent.h>
#include <ctime>
#include <sys/stat.h>
#endif
#include "rtklib.h"
#include <string>
static const double gpst0[]={1980,1, 6,0,0,0}; /* gps time reference */
static const double gst0 []={1999,8,22,0,0,0}; /* galileo system time reference */
static const double bdt0 []={2006,1, 1,0,0,0}; /* beidou time reference */

static const unsigned int tbl_CRC24Q[]={
        0x000000,0x864CFB,0x8AD50D,0x0C99F6,0x93E6E1,0x15AA1A,0x1933EC,0x9F7F17,
        0xA18139,0x27CDC2,0x2B5434,0xAD18CF,0x3267D8,0xB42B23,0xB8B2D5,0x3EFE2E,
        0xC54E89,0x430272,0x4F9B84,0xC9D77F,0x56A868,0xD0E493,0xDC7D65,0x5A319E,
        0x64CFB0,0xE2834B,0xEE1ABD,0x685646,0xF72951,0x7165AA,0x7DFC5C,0xFBB0A7,
        0x0CD1E9,0x8A9D12,0x8604E4,0x00481F,0x9F3708,0x197BF3,0x15E205,0x93AEFE,
        0xAD50D0,0x2B1C2B,0x2785DD,0xA1C926,0x3EB631,0xB8FACA,0xB4633C,0x322FC7,
        0xC99F60,0x4FD39B,0x434A6D,0xC50696,0x5A7981,0xDC357A,0xD0AC8C,0x56E077,
        0x681E59,0xEE52A2,0xE2CB54,0x6487AF,0xFBF8B8,0x7DB443,0x712DB5,0xF7614E,
        0x19A3D2,0x9FEF29,0x9376DF,0x153A24,0x8A4533,0x0C09C8,0x00903E,0x86DCC5,
        0xB822EB,0x3E6E10,0x32F7E6,0xB4BB1D,0x2BC40A,0xAD88F1,0xA11107,0x275DFC,
        0xDCED5B,0x5AA1A0,0x563856,0xD074AD,0x4F0BBA,0xC94741,0xC5DEB7,0x43924C,
        0x7D6C62,0xFB2099,0xF7B96F,0x71F594,0xEE8A83,0x68C678,0x645F8E,0xE21375,
        0x15723B,0x933EC0,0x9FA736,0x19EBCD,0x8694DA,0x00D821,0x0C41D7,0x8A0D2C,
        0xB4F302,0x32BFF9,0x3E260F,0xB86AF4,0x2715E3,0xA15918,0xADC0EE,0x2B8C15,
        0xD03CB2,0x567049,0x5AE9BF,0xDCA544,0x43DA53,0xC596A8,0xC90F5E,0x4F43A5,
        0x71BD8B,0xF7F170,0xFB6886,0x7D247D,0xE25B6A,0x641791,0x688E67,0xEEC29C,
        0x3347A4,0xB50B5F,0xB992A9,0x3FDE52,0xA0A145,0x26EDBE,0x2A7448,0xAC38B3,
        0x92C69D,0x148A66,0x181390,0x9E5F6B,0x01207C,0x876C87,0x8BF571,0x0DB98A,
        0xF6092D,0x7045D6,0x7CDC20,0xFA90DB,0x65EFCC,0xE3A337,0xEF3AC1,0x69763A,
        0x578814,0xD1C4EF,0xDD5D19,0x5B11E2,0xC46EF5,0x42220E,0x4EBBF8,0xC8F703,
        0x3F964D,0xB9DAB6,0xB54340,0x330FBB,0xAC70AC,0x2A3C57,0x26A5A1,0xA0E95A,
        0x9E1774,0x185B8F,0x14C279,0x928E82,0x0DF195,0x8BBD6E,0x872498,0x016863,
        0xFAD8C4,0x7C943F,0x700DC9,0xF64132,0x693E25,0xEF72DE,0xE3EB28,0x65A7D3,
        0x5B59FD,0xDD1506,0xD18CF0,0x57C00B,0xC8BF1C,0x4EF3E7,0x426A11,0xC426EA,
        0x2AE476,0xACA88D,0xA0317B,0x267D80,0xB90297,0x3F4E6C,0x33D79A,0xB59B61,
        0x8B654F,0x0D29B4,0x01B042,0x87FCB9,0x1883AE,0x9ECF55,0x9256A3,0x141A58,
        0xEFAAFF,0x69E604,0x657FF2,0xE33309,0x7C4C1E,0xFA00E5,0xF69913,0x70D5E8,
        0x4E2BC6,0xC8673D,0xC4FECB,0x42B230,0xDDCD27,0x5B81DC,0x57182A,0xD154D1,
        0x26359F,0xA07964,0xACE092,0x2AAC69,0xB5D37E,0x339F85,0x3F0673,0xB94A88,
        0x87B4A6,0x01F85D,0x0D61AB,0x8B2D50,0x145247,0x921EBC,0x9E874A,0x18CBB1,
        0xE37B16,0x6537ED,0x69AE1B,0xEFE2E0,0x709DF7,0xF6D10C,0xFA48FA,0x7C0401,
        0x42FA2F,0xC4B6D4,0xC82F22,0x4E63D9,0xD11CCE,0x575035,0x5BC9C3,0xDD8538
};
static double leaps[MAXLEAPS+1][7]={ /* leap seconds (y,m,d,h,m,s,utc-gpst) */
        {2017,1,1,0,0,0,-18},
        {2015,7,1,0,0,0,-17},
        {2012,7,1,0,0,0,-16},
        {2009,1,1,0,0,0,-15},
        {2006,1,1,0,0,0,-14},
        {1999,1,1,0,0,0,-13},
        {1997,7,1,0,0,0,-12},
        {1996,1,1,0,0,0,-11},
        {1994,7,1,0,0,0,-10},
        {1993,7,1,0,0,0, -9},
        {1992,7,1,0,0,0, -8},
        {1991,1,1,0,0,0, -7},
        {1990,1,1,0,0,0, -6},
        {1988,1,1,0,0,0, -5},
        {1985,7,1,0,0,0, -4},
        {1983,7,1,0,0,0, -3},
        {1982,7,1,0,0,0, -2},
        {1981,7,1,0,0,0, -1},
        {0}
};
static std::string obscodes[]={       /* observation code strings */

    ""  ,"1C","1P","1W","1Y", "1M","1N","1S","1L","1E", /*  0- 9 */
    "1A","1B","1X","1Z","2C", "2D","2S","2L","2X","2P", /* 10-19 */
    "2W","2Y","2M","2N","5I", "5Q","5X","7I","7Q","7X", /* 20-29 */
    "6A","6B","6C","6X","6Z", "6S","6L","8L","8Q","8X", /* 30-39 */
    "2I","2Q","6I","6Q","3I", "3Q","3X","1I","1Q","5A", /* 40-49 */
    "5B","5C","9A","9B","9C", "9X","1D","5D","5P","5Z", /* 50-59 */
    "6E","7D","7P","7Z","8D", "8P","4A","4B","4X",""    /* 60-69 */
};

/* satellite system+prn/slot number to satellite number ------------------------
* convert satellite system+prn/slot number to satellite number
* args   : int    sys       I   satellite system (SYS_GPS,SYS_GLO,...)
*          int    prn       I   satellite prn/slot number
* return : satellite number (0:error)
*-----------------------------------------------------------------------------*/
extern int satno(int sys, int prn)
{
  if (prn<=0) return 0;
  switch (sys) {
  case SYS_GPS:
    if (prn < MINPRNGPS || MAXPRNGPS < prn)
      return 0;
    return prn - MINPRNGPS + 1;
  case SYS_GAL:
    if (prn < MINPRNGAL || MAXPRNGAL < prn)
      return 0;
    return NSATGPS + prn - MINPRNGAL + 1;
  case SYS_CMP:
    if (prn < MINPRNCMP || MAXPRNCMP < prn)
      return 0;
    return NSATGPS + NSATGAL + prn - MINPRNCMP + 1;
  }
  return 0;
}
/* satellite number to satellite system ----------------------------------------
* convert satellite number to satellite system
* args   : int    sat       I   satellite number (1-MAXSAT)
*          int    *prn      IO  satellite prn/slot number (NULL: no output)
* return : satellite system (SYS_GPS,SYS_GLO,...)
*-----------------------------------------------------------------------------*/
extern int satsys(int sat, int *prn)
{
    int sys=SYS_NONE;
    if (sat<=0||MAXSAT<sat) sat=0;
    else if (sat<=NSATGPS) {
        sys=SYS_GPS; sat+=MINPRNGPS-1;
    }
    else if ((sat-=NSATGPS)<=NSATGAL) {
        sys=SYS_GAL; sat+=MINPRNGAL-1;
    }
    else if ((sat-=NSATGAL)<=NSATCMP) {
        sys=SYS_CMP; sat+=MINPRNCMP-1;
    }
    else sat=0;
    if (prn) *prn=sat;
    return sys;
}
/* obs code to obs code string -------------------------------------------------
* convert obs code to obs code string
* args   : unsigned char code I obs code (CODE_???)
*          int    *freq  IO     frequency (NULL: no output)
*                               (1:L1/E1, 2:L2/B1, 3:L5/E5a/L3, 4:L6/LEX/B3,
                                 5:E5b/B2, 6:E5(a+b), 7:S)
* return : obs code string ("1C","1P","1P",...)
* notes  : obs codes are based on reference [6] and qzss extension
*-----------------------------------------------------------------------------*/
extern std::string code2obs(uint8_t code)
{
  if (code<=CODE_NONE||MAXCODE<code) return "";
  return obscodes[code];
}
/* GPS obs code to frequency -------------------------------------------------*/
static int code2freq_GPS(uint8_t code, double *freq)
{
  std::string obs=code2obs(code);

  switch (obs[0]) {
  case '1': *freq=FREQL1; return 0; /* L1 */
  case '2': *freq=FREQL2; return 1; /* L2 */
  case '5': *freq=FREQL5; return 2; /* L5 */
  }
  return -1;
}
/* Galileo obs code to frequency ---------------------------------------------*/
static int code2freq_GAL(uint8_t code, double *freq)
{
  std::string obs=code2obs(code);

  switch (obs[0]) {
  case '1': *freq=FREQL1; return 0; /* E1 */
  case '7': *freq=FREQE5b; return 1; /* E5b */
  case '5': *freq=FREQL5; return 2; /* E5a */
  case '6': *freq=FREQE6; return 3; /* E6 */
  case '8': *freq=FREQE5ab; return 4; /* E5ab */
  }
  return -1;
}
/* BDS obs code to frequency -------------------------------------------------*/
static int code2freq_BDS(uint8_t code, double *freq)
{
  std::string obs=code2obs(code);

  switch (obs[0]) {
  case '1': *freq=FREQL1;     return 0; /* B1C */
  case '2': *freq=FREQ1_CMP; return 0; /* B1-2 */
  case '7': *freq=FREQ2_CMP; return 1; /* B2I/B2b */
  case '5': *freq=FREQL5;     return 2; /* B2a */
  case '6': *freq=FREQ3_CMP; return 3; /* B3 */
  case '8': *freq=FREQE5ab;     return 4; /* B2ab */
  }
  return -1;
}
/* system and obs code to frequency --------------------------------------------
* convert system and obs code to carrier frequency
* args   : int    sys       I   satellite system (SYS_???)
*          uint8_t code     I   obs code (CODE_???)
*          int    fcn       I   frequency channel number for GLONASS
* return : carrier frequency (Hz) (0.0: error)
*-----------------------------------------------------------------------------*/
extern double code2freq(int sys, uint8_t code, int fcn)
{
  double freq=0.0;

  switch (sys) {
  case SYS_GPS: (void)code2freq_GPS(code,&freq); break;
  //case SYS_GLO: (void)code2freq_GLO(code,fcn,&freq); break;
  case SYS_GAL: (void)code2freq_GAL(code,&freq); break;
  case SYS_CMP: (void)code2freq_BDS(code,&freq); break;
  }
  return freq;
}

/* set unsigned/signed bits ----------------------------------------------------
* set unsigned/signed bits to byte data
* args   : unsigned char *buff IO byte data
*          int    pos    I      bit position from start of data (bits)
*          int    len    I      bit length (bits) (len<=32)
*         (unsigned) int I      unsigned/signed data
* return : none
*-----------------------------------------------------------------------------*/
extern void setbitu(uint8_t *buff, int pos, int len, uint32_t data)
{
    unsigned int mask=1u<<(len-1);
    int i;
    if (len<=0||32<len) return;
    for (i=pos;i<pos+len;i++,mask>>=1) {
        if (data&mask) buff[i/8]|=1u<<(7-i%8); else buff[i/8]&=~(1u<<(7-i%8));
    }
}
extern void setbits(uint8_t *buff, int pos, int len, int32_t data)
{
    if (data<0) data|=1<<(len-1); else data&=~(1<<(len-1)); /* set sign bit */
    setbitu(buff,pos,len,(unsigned int)data);
}
/* crc-24q parity --------------------------------------------------------------
* compute crc-24q parity for sbas, rtcm3
* args   : unsigned char *buff I data
*          int    len    I      data length (bytes)
* return : crc-24Q parity
* notes  : see reference [2] A.4.3.3 Parity
*-----------------------------------------------------------------------------*/
extern uint32_t rtk_crc24q(const uint8_t *buff, int len)
{
    unsigned int crc=0;
    int i;

    //trace(4,"rtk_crc24q: len=%d\n",len);

    for (i=0;i<len;i++) crc=((crc<<8)&0xFFFFFF)^tbl_CRC24Q[(crc>>16)^buff[i]];
    return crc;
}
/* convert calendar day/time to time -------------------------------------------
* convert calendar day/time to gtime_t struct
* args   : double *ep       I   day/time {year,month,day,hour,min,sec}
* return : gtime_t struct
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
extern gtime_t epoch2time(const double *ep)
{
    const int doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
    gtime_t time={0};
    int days,sec,year=(int)ep[0],mon=(int)ep[1],day=(int)ep[2];

    if (year<1970||2099<year||mon<1||12<mon) return time;

    /* leap year if year%4==0 in 1901-2099 */
    days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
    sec=(int)floor(ep[5]);
    time.time=(time_t)days*86400+(int)ep[3]*3600+(int)ep[4]*60+sec;
    time.sec=ep[5]-sec;
    return time;
}
extern gtime_t epoch2time(std::vector<double> ep)
{
  const int doy[]={1,32,60,91,121,152,182,213,244,274,305,335};
  gtime_t time={0};
  int days,sec,year=(int)ep[0],mon=(int)ep[1],day=(int)ep[2];

  if (year<1970||2099<year||mon<1||12<mon) return time;

  /* leap year if year%4==0 in 1901-2099 */
  days=(year-1970)*365+(year-1969)/4+doy[mon-1]+day-2+(year%4==0&&mon>=3?1:0);
  sec=(int)floor(ep[5]);
  time.time=(time_t)days*86400+(int)ep[3]*3600+(int)ep[4]*60+sec;
  time.sec=ep[5]-sec;
  return time;
}
/* add time --------------------------------------------------------------------
* add time to gtime_t struct
* args   : gtime_t t        I   gtime_t struct
*          double sec       I   time to add (s)
* return : gtime_t struct (t+sec)
*-----------------------------------------------------------------------------*/
extern gtime_t timeadd(gtime_t t, double sec)
{
    double tt;

    t.sec+=sec; tt=floor(t.sec); t.time+=(int)tt; t.sec-=tt;
    return t;
}
/* time to gps time ------------------------------------------------------------
* convert gtime_t struct to week and tow in gps time
* args   : gtime_t t        I   gtime_t struct
*          int    *week     IO  week number in gps time (NULL: no output)
* return : time of week in gps time (s)
*-----------------------------------------------------------------------------*/
extern double time2gpst(gtime_t t, int *week)
{
    gtime_t t0=epoch2time(gpst0);
    time_t sec=t.time-t0.time;
    int w=(int)(sec/(86400*7));

    if (week) *week=w;
    return (double)(sec-(double)w*86400*7)+t.sec;
}
/* gps time to time ------------------------------------------------------------
* convert week and tow in gps time to gtime_t struct
* args   : int    week      I   week number in gps time
*          double sec       I   time of week in gps time (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
extern gtime_t gpst2time(int week, double sec)
{
  gtime_t t=epoch2time(gpst0);

  if (sec<-1E9||1E9<sec) sec=0.0;
  t.time+=(time_t)86400*7*week+(int)sec;
  t.sec=sec-(int)sec;
  return t;
}
/* galileo system time to time -------------------------------------------------
* convert week and tow in galileo system time (gst) to gtime_t struct
* args   : int    week      I   week number in gst
*          double sec       I   time of week in gst (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
extern gtime_t gst2time(int week, double sec)
{
  gtime_t t=epoch2time(gst0);

  if (sec<-1E9||1E9<sec) sec=0.0;
  t.time+=(time_t)86400*7*week+(int)sec;
  t.sec=sec-(int)sec;
  return t;
}
/* beidou time (bdt) to time ---------------------------------------------------
* convert week and tow in beidou time (bdt) to gtime_t struct
* args   : int    week      I   week number in bdt
*          double sec       I   time of week in bdt (s)
* return : gtime_t struct
*-----------------------------------------------------------------------------*/
extern gtime_t bdt2time(int week, double sec)
{
  gtime_t t=epoch2time(bdt0);

  if (sec<-1E9||1E9<sec) sec=0.0;
  t.time+=(time_t)86400*7*week+(int)sec;
  t.sec=sec-(int)sec;
  return t;
}
/* gpstime to bdt --------------------------------------------------------------
* convert gpstime to bdt (beidou navigation satellite system time)
* args   : gtime_t t        I   time expressed in gpstime
* return : time expressed in bdt
* notes  : ref [8] 3.3, 2006/1/1 00:00 BDT = 2006/1/1 00:00 UTC
*          no leap seconds in BDT
*          ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t gpst2bdt(gtime_t t)
{
  return timeadd(t,-14.0);
}
/* bdt to gpstime --------------------------------------------------------------
* convert bdt (beidou navigation satellite system time) to gpstime
* args   : gtime_t t        I   time expressed in bdt
* return : time expressed in gpstime
* notes  : see gpst2bdt()
*-----------------------------------------------------------------------------*/
extern gtime_t bdt2gpst(gtime_t t)
{
  return timeadd(t,14.0);
}
/* gpstime to utc --------------------------------------------------------------
* convert gpstime to utc considering leap seconds
* args   : gtime_t t        I   time expressed in gpstime
* return : time expressed in utc
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t gpst2utc(gtime_t t)
{
    gtime_t tu;
    int i;

    for (i=0;leaps[i][0]>0;i++) {
        tu=timeadd(t,leaps[i][6]);
        if (timediff(tu,epoch2time(leaps[i]))>=0.0) return tu;
    }
    return t;
}
/* utc to gpstime --------------------------------------------------------------
* convert utc to gpstime considering leap seconds
* args   : gtime_t t        I   time expressed in utc
* return : time expressed in gpstime
* notes  : ignore slight time offset under 100 ns
*-----------------------------------------------------------------------------*/
extern gtime_t utc2gpst(gtime_t t)
{
  int i;

  for (i=0;leaps[i][0]>0;i++) {
    if (timediff(t,epoch2time(leaps[i]))>=0.0) return timeadd(t,-leaps[i][6]);
  }
  return t;
}
/* time to calendar day/time ---------------------------------------------------
* convert gtime_t struct to calendar day/time
* args   : gtime_t t        I   gtime_t struct
*          double *ep       O   day/time {year,month,day,hour,min,sec}
* return : none
* notes  : proper in 1970-2037 or 1970-2099 (64bit time_t)
*-----------------------------------------------------------------------------*/
extern void time2epoch(gtime_t t, double *ep)
{
  const int mday[]={ /* # of days in a month */
          31,28,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31,
          31,29,31,30,31,30,31,31,30,31,30,31,31,28,31,30,31,30,31,31,30,31,30,31
  };
  int days,sec,mon,day;

  /* leap year if year%4==0 in 1901-2099 */
  days=(int)(t.time/86400);
  sec=(int)(t.time-(time_t)days*86400);
  for (day=days%1461,mon=0;mon<48;mon++) {
    if (day>=mday[mon]) day-=mday[mon]; else break;
  }
  ep[0]=1970+days/1461*4+mon/12; ep[1]=mon%12+1; ep[2]=day+1;
  ep[3]=sec/3600; ep[4]=sec%3600/60; ep[5]=sec%60+t.sec;
}
/* time to day and sec -------------------------------------------------------*/
static double time2sec(gtime_t time, gtime_t *day)
{
  double ep[6],sec;
  time2epoch(time,ep);
  sec=ep[3]*3600.0+ep[4]*60.0+ep[5];
  ep[3]=ep[4]=ep[5]=0.0;
  *day=epoch2time(ep);
  return sec;
}
/* utc to gmst -----------------------------------------------------------------
* convert utc to gmst (Greenwich mean sidereal time)
* args   : gtime_t t        I   time expressed in utc
*          double ut1_utc   I   UT1-UTC (s)
* return : gmst (rad)
*-----------------------------------------------------------------------------*/
extern double utc2gmst(gtime_t t, double ut1_utc)
{
  const double ep2000[]={2000,1,1,12,0,0};
  gtime_t tut,tut0;
  double ut,t1,t2,t3,gmst0,gmst;

  tut=timeadd(t,ut1_utc);
  ut=time2sec(tut,&tut0);
  t1=timediff(tut0,epoch2time(ep2000))/86400.0/36525.0;
  t2=t1*t1; t3=t2*t1;
  gmst0=24110.54841+8640184.812866*t1+0.093104*t2-6.2E-6*t3;
  gmst=gmst0+1.002737909350795*ut;

  return fmod(gmst,86400.0)*PI/43200.0; /* 0 <= gmst <= 2*PI */
}
/* time difference -------------------------------------------------------------
* difference between gtime_t structs
* args   : gtime_t t1,t2    I   gtime_t structs
* return : time difference (t1-t2) (s)
*-----------------------------------------------------------------------------*/
extern double timediff(gtime_t t1, gtime_t t2)
{
    return difftime(t1.time,t2.time)+t1.sec-t2.sec;
}
/* sort and unique observation data --------------------------------------------
* sort and unique observation data by time, rcv, sat
* args   : obs_t *obs    IO     observation data
* return : number of epochs
*-----------------------------------------------------------------------------*/
/* compare observation data -------------------------------------------------*/
static int cmpobs(const void *p1, const void *p2)
{
    obsd_t *q1=(obsd_t *)p1,*q2=(obsd_t *)p2;
    double tt=timediff(q1->time,q2->time);
    if (fabs(tt)>DTTOL) return tt<0?-1:1;
    if (q1->rcv!=q2->rcv) return (int)q1->rcv-(int)q2->rcv;
    return (int)q1->sat-(int)q2->sat;
}
extern int sortobs(obs_t *obs)
{
    int i,j,n;

    //trace(3,"sortobs: nobs=%d\n",obs->n);

    if (obs->n<=0) return 0;

    qsort(obs->data,obs->n,sizeof(obsd_t),cmpobs);

    /* delete duplicated data */
    for (i=j=0;i<obs->n;i++) {
        if (obs->data[i].sat!=obs->data[j].sat||
            obs->data[i].rcv!=obs->data[j].rcv||
            timediff(obs->data[i].time,obs->data[j].time)!=0.0) {
            obs->data[++j]=obs->data[i];
        }
    }
    obs->n=j+1;

    for (i=n=0;i<obs->n;i=j,n++) {
        for (j=i+1;j<obs->n;j++) {
            if (timediff(obs->data[j].time,obs->data[i].time)>DTTOL) break;
        }
    }
    return n;
}
/* free navigation data ---------------------------------------------------------
* free memory for navigation data
* args   : nav_t *nav    IO     navigation data
*          int   opt     I      option (or of followings)
*                               (0x01: gps/qzs ephmeris, 0x02: glonass ephemeris,
*                                0x04: sbas ephemeris,   0x08: precise ephemeris,
*                                0x10: precise clock     0x20: almanac,
*                                0x40: tec data)
* return : none
*-----------------------------------------------------------------------------*/
extern void freenav(nav_t *nav, int opt)
{
    if (opt&0x01) {free(nav->eph ); nav->eph =NULL; nav->n =nav->nmax =0;}
    if (opt&0x02) {free(nav->geph); nav->geph=NULL; nav->ng=nav->ngmax=0;}
    if (opt&0x04) {free(nav->seph); nav->seph=NULL; nav->ns=nav->nsmax=0;}
    if (opt&0x08) {free(nav->peph); nav->peph=NULL; nav->ne=nav->nemax=0;}
    if (opt&0x10) {free(nav->pclk); nav->pclk=NULL; nav->nc=nav->ncmax=0;}
    if (opt&0x20) {free(nav->alm ); nav->alm =NULL; nav->na=nav->namax=0;}
    if (opt&0x40) {free(nav->tec ); nav->tec =NULL; nav->nt=nav->ntmax=0;}
}

/* transform ecef to geodetic postion ------------------------------------------
* transform ecef position to geodetic position
* args   : double *r        I   ecef position {x,y,z} (m)
*          double *pos      O   geodetic position {lat,lon,h} (rad,m)
* return : none
* notes  : WGS84, ellipsoidal height in meter
*-----------------------------------------------------------------------------*/
extern void ecef2pos(const double *r, double *pos)
{
  double e2=FE_WGS84*(2.0-FE_WGS84),r2=dot(r,r,2),z,zk,v=RE_WGS84,sinp;

  for (z=r[2],zk=0.0;fabs(z-zk)>=1E-4;) {
    zk=z;
    sinp=z/sqrt(r2+z*z);
    v=RE_WGS84/sqrt(1.0-e2*sinp*sinp);
    z=r[2]+v*e2*sinp;
  }
  pos[0]=r2>1E-12?atan(z/sqrt(r2)):(r[2]>0.0?PI/2.0:-PI/2.0);
  pos[1]=r2>1E-12?atan2(r[1],r[0]):0.0;
  pos[2]=sqrt(r2+z*z)-v;
}
/* transform geodetic to ecef position -----------------------------------------
* transform geodetic position to ecef position
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *r        O   ecef position {x,y,z} (m)
* return : none
* notes  : WGS84, ellipsoidal height in meter
*-----------------------------------------------------------------------------*/
extern void pos2ecef(const double *pos, double *r)
{
  double sinp=sin(pos[0]),cosp=cos(pos[0]),sinl=sin(pos[1]),cosl=cos(pos[1]);
  double e2=FE_WGS84*(2.0-FE_WGS84),v=RE_WGS84/sqrt(1.0-e2*sinp*sinp);

  r[0]=(v+pos[2])*cosp*cosl;
  r[1]=(v+pos[2])*cosp*sinl;
  r[2]=(v*(1.0-e2)+pos[2])*sinp;
}
/* ecef to local coordinate transfromation matrix ------------------------------
* compute ecef to local coordinate transfromation matrix
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *E        O   ecef to local coord transformation matrix (3x3)
* return : none
* notes  : matirix stored by column-major order (fortran convention)
*-----------------------------------------------------------------------------*/
extern void xyz2enu(const double *pos, double *E)
{
  double sinp=sin(pos[0]),cosp=cos(pos[0]),sinl=sin(pos[1]),cosl=cos(pos[1]);

  E[0]=-sinl;      E[3]=cosl;       E[6]=0.0;
  E[1]=-sinp*cosl; E[4]=-sinp*sinl; E[7]=cosp;
  E[2]=cosp*cosl;  E[5]=cosp*sinl;  E[8]=sinp;
}
/* transform ecef vector to local tangental coordinate -------------------------
* transform ecef vector to local tangental coordinate
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *r        I   vector in ecef coordinate {x,y,z}
*          double *e        O   vector in local tangental coordinate {e,n,u}
* return : none
*-----------------------------------------------------------------------------*/
extern void ecef2enu(const double *pos, const double *r, double *e)
{
  double E[9];

  xyz2enu(pos,E);
  matmul("NN",3,1,3,1.0,E,r,0.0,e);
}
/* transform local vector to ecef coordinate -----------------------------------
* transform local tangental coordinate vector to ecef
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *e        I   vector in local tangental coordinate {e,n,u}
*          double *r        O   vector in ecef coordinate {x,y,z}
* return : none
*-----------------------------------------------------------------------------*/
extern void enu2ecef(const double *pos, const double *e, double *r)
{
  double E[9];

  xyz2enu(pos,E);
  matmul("TN",3,1,3,1.0,E,e,0.0,r);
}
/* transform covariance to local tangental coordinate --------------------------
* transform ecef covariance to local tangental coordinate
* args   : double *pos      I   geodetic position {lat,lon} (rad)
*          double *P        I   covariance in ecef coordinate
*          double *Q        O   covariance in local tangental coordinate
* return : none
*-----------------------------------------------------------------------------*/
extern void covenu(const double *pos, const double *P, double *Q)
{
  double E[9],EP[9];

  xyz2enu(pos,E);
  matmul("NN",3,3,3,1.0,E,P,0.0,EP);
  matmul("NT",3,3,3,1.0,EP,E,0.0,Q);
}

/* multiply matrix -----------------------------------------------------------*/
extern void matmul(const char *tr, int n, int k, int m, double alpha,
                   const double *A, const double *B, double beta, double *C)
{
  double d;
  int i,j,x,f=tr[0]=='N'?(tr[1]=='N'?1:2):(tr[1]=='N'?3:4);

  for (i=0;i<n;i++) for (j=0;j<k;j++) {
      d=0.0;
      switch (f) {
        case 1: for (x=0;x<m;x++) d+=A[i+x*n]*B[x+j*m]; break;
        case 2: for (x=0;x<m;x++) d+=A[i+x*n]*B[j+x*k]; break;
        case 3: for (x=0;x<m;x++) d+=A[x+i*m]*B[x+j*m]; break;
        case 4: for (x=0;x<m;x++) d+=A[x+i*m]*B[j+x*k]; break;
      }
      if (beta==0.0) C[i+j*n]=alpha*d; else C[i+j*n]=alpha*d+beta*C[i+j*n];
    }
}
/* copy matrix -----------------------------------------------------------------
* copy matrix
* args   : double *A        O   destination matrix A (n x m)
*          double *B        I   source matrix B (n x m)
*          int    n,m       I   number of rows and columns of matrix
* return : none
*-----------------------------------------------------------------------------*/
extern void matcpy(double *A, const double *B, int n, int m)
{
  memcpy(A,B,sizeof(double)*n*m);
}
/* outer product of 3d vectors -------------------------------------------------
* outer product of 3d vectors
* args   : double *a,*b     I   vector a,b (3 x 1)
*          double *c        O   outer product (a x b) (3 x 1)
* return : none
*-----------------------------------------------------------------------------*/
extern void cross3(const double *a, const double *b, double *c)
{
  c[0]=a[1]*b[2]-a[2]*b[1];
  c[1]=a[2]*b[0]-a[0]*b[2];
  c[2]=a[0]*b[1]-a[1]*b[0];
}
/* normalize 3d vector ---------------------------------------------------------
* normalize 3d vector
* args   : double *a        I   vector a (3 x 1)
*          double *b        O   normlized vector (3 x 1) || b || = 1
* return : status (1:ok,0:error)
*-----------------------------------------------------------------------------*/
extern int normv3(const double *a, double *b)
{
  double r;
  if ((r=norm(a,3))<=0.0) return 0;
  b[0]=a[0]/r;
  b[1]=a[1]/r;
  b[2]=a[2]/r;
  return 1;
}
/* inner product ---------------------------------------------------------------
* inner product of vectors
* args   : double *a,*b     I   vector a,b (n x 1)
*          int    n         I   size of vector a,b
* return : a'*b
*-----------------------------------------------------------------------------*/
extern double dot(const double *a, const double *b, int n)
{
  double c=0.0;

  while (--n>=0) c+=a[n]*b[n];
  return c;
}
/* euclid norm -----------------------------------------------------------------
* euclid norm of vector
* args   : double *a        I   vector a (n x 1)
*          int    n         I   size of vector a
* return : || a ||
*-----------------------------------------------------------------------------*/
extern double norm(const double *a, int n)
{
  return sqrt(dot(a,a,n));
}