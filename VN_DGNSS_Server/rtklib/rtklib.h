
#ifndef WADGNSS_SERVER_RTKLIB_H
#define WADGNSS_SERVER_RTKLIB_H
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <iostream>
#include <cmath>
#include <time.h>
#include <cctype>
#include <vector>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#endif
//#ifdef __cplusplus
//extern "C" {
//#endif

#ifdef WIN_DLL
#define EXPORT __declspec(dllexport) /* for Windows DLL */
#else
#define EXPORT
#endif
/* constants -----------------------------------------------------------------*/
#define Eps 1e-8
#define VER_RTKLIB  "2.4.3"             /* library version */

#define PATCH_LEVEL "b33"               /* patch level */

#define COPYRIGHT_RTKLIB \
            "Copyright (C) 2007-2020 T.Takasu\nAll rights reserved."

#define PI          3.1415926535897932  /* pi */
#define D2R         (PI/180.0)          /* deg to rad */
#define R2D         (180.0/PI)          /* rad to deg */
#define CLIGHT      299792458.0         /* speed of light (m/s) */
#define SC2RAD      3.1415926535898     /* semi-circle to radian (IS-GPS) */
#define AU          149597870691.0      /* 1 AU (m) */
#define AS2R        (D2R/3600.0)        /* arc sec to radian */

#define OMGE        7.2921151467E-5     /* earth angular velocity (IS-GPS) (rad/s) */

#define RE_WGS84    6378137.0           /* earth semimajor axis (WGS84) (m) */
#define FE_WGS84    (1.0/298.257223563) /* earth flattening (WGS84) */

#define HION        350000.0            /* ionosphere height (m) */

#define MAXFREQ     7                   /* max NFREQ */

#define FREQL1      1.57542E9           /* L1/E1/B1C  frequency 1 (Hz) */
#define FREQL2      1.22760E9           /* L2     frequency 2 (Hz) */
#define FREQE5b     1.20714E9           /* E5b    frequency 3 (Hz) */
#define FREQL5      1.17645E9           /* L5/E5a/B2a frequency 4 (Hz) */
#define FREQE6      1.27875E9           /* E6/LEX frequency 5 (Hz) */
#define FREQE5ab    1.191795E9          /* E5a+b  frequency 6 (Hz) */
#define FREQs       2.492028E9           /* S      frequency 7 (Hz) */
#define FREQ1_GLO   1.60200E9           /* GLONASS G1 base frequency (Hz) */
#define DFRQ1_GLO   0.56250E6           /* GLONASS G1 bias frequency (Hz/n) */
#define FREQ2_GLO   1.24600E9           /* GLONASS G2 base frequency (Hz) */
#define DFRQ2_GLO   0.43750E6           /* GLONASS G2 bias frequency (Hz/n) */
#define FREQ3_GLO   1.202025E9          /* GLONASS G3 frequency (Hz) */
#define FREQ1_CMP   1.561098E9          /* BDS B1-2     frequency (Hz) */
#define FREQ2_CMP   1.20714E9           /* BDS B2I/B2b frequency (Hz) */
#define FREQ3_CMP   1.26852E9           /* BDS B3      frequency (Hz) */
#define SYS_NONE    0x00                /* navigation system: none */
#define SYS_GPS     0x01                /* navigation system: GPS */
#define SYS_SBS     0x02                /* navigation system: SBAS */
#define SYS_GLO     0x04                /* navigation system: GLONASS */
#define SYS_GAL     0x08                /* navigation system: Galileo */
#define SYS_QZS     0x10                /* navigation system: QZSS */
#define SYS_CMP     0x20                /* navigation system: BeiDou */
#define SYS_IRN     0x40                /* navigation system: IRNS */
#define SYS_LEO     0x80                /* navigation system: LEO */
#define SYS_ALL     0xFF                /* navigation system: all */
#ifndef NFREQ
#define NFREQ       3                   /* number of carrier frequencies */
#endif
#define NFREQGLO    2                   /* number of carrier frequencies of GLONASS */

#ifndef NEXOBS
#define NEXOBS      0                   /* number of extended obs codes */
#endif

#define MINPRNGPS   1                   /* min satellite PRN number of GPS */
#define MAXPRNGPS   32                  /* max satellite PRN number of GPS */
#define NSATGPS     (MAXPRNGPS-MINPRNGPS+1) /* number of GPS satellites */
#define NSYSGPS     1

#define MINPRNGLO   1                   /* min satellite slot number of GLONASS */
#define MAXPRNGLO   27                  /* max satellite slot number of GLONASS */
#define NSATGLO     (MAXPRNGLO-MINPRNGLO+1) /* number of GLONASS satellites */
#define NSYSGLO     1

#define MINPRNGAL   1                   /* min satellite PRN number of Galileo */
#define MAXPRNGAL   36                  /* max satellite PRN number of Galileo */
#define NSATGAL    (MAXPRNGAL-MINPRNGAL+1) /* number of Galileo satellites */
#define NSYSGAL     1

#define MINPRNCMP   1                   /* min satellite sat number of BeiDou */
#define MAXPRNCMP   63                  /* max satellite sat number of BeiDou */
#define NSATCMP     (MAXPRNCMP-MINPRNCMP+1) /* number of BeiDou satellites */
#define NSYSCMP     1

#define NSYS        (NSYSGPS+NSYSGLO+NSYSGAL+NSYSCMP) /* number of systems */

#define MAXSAT      (NSATGPS+NSATGAL+NSATCMP)
/* max satellite number (1 to MAXSAT) */
#define MAXSTA      255

#ifndef MAXOBS
#define MAXOBS      96                  /* max number of obs in an epoch */
#endif
#define MAXRCV      64                  /* max receiver number (1 to MAXRCV) */
#define MAXOBSTYPE  64                  /* max number of obs type in RINEX */
#ifdef OBS_100HZ
#define DTTOL       0.005               /* tolerance of time difference (s) */
#else
#define DTTOL       0.025               /* tolerance of time difference (s) */
#endif
#define MAXDTOE     7200.0              /* max time difference to GPS Toe (s) */
#define MAXDTOE_QZS 7200.0              /* max time difference to QZSS Toe (s) */
#define MAXDTOE_GAL 14400.0             /* max time difference to Galileo Toe (s) */
#define MAXDTOE_CMP 21600.0             /* max time difference to BeiDou Toe (s) */
#define MAXDTOE_GLO 1800.0              /* max time difference to GLONASS Toe (s) */
#define MAXDTOE_SBS 360.0               /* max time difference to SBAS Toe (s) */
#define MAXDTOE_S   86400.0             /* max time difference to ephem toe (s) for other */
#define MAXGDOP     300.0               /* max GDOP */

#define INT_SWAP_TRAC 86400.0           /* swap interval of trace file (s) */
#define INT_SWAP_STAT 86400.0           /* swap interval of solution status file (s) */

#define MAXEXFILE   1024                /* max number of expanded files */
#define MAXSBSAGEF  30.0                /* max age of SBAS fast correction (s) */
#define MAXSBSAGEL  1800.0              /* max age of SBAS long term corr (s) */
#define MAXSBSURA   8                   /* max URA of SBAS satellite */
#define MAXBAND     10                  /* max SBAS band of IGP */
#define MAXNIGP     201                 /* max number of IGP in SBAS band */
#define MAXNGEO     4                   /* max number of GEO satellites */
#define MAXCOMMENT  100                  /* max number of RINEX comments */
#define MAXSTRPATH  1024                /* max length of stream path */
#define MAXSTRMSG   1024                /* max length of stream message */
#define MAXSTRRTK   8                   /* max number of stream in RTK server */
#define MAXSBSMSG   32                  /* max number of SBAS msg in RTK server */
#define MAXSOLMSG   8191                /* max length of solution message */
#define MAXRAWLEN   16384                /* max length of receiver raw message */
#define MAXERRMSG   4096                /* max length of error/warning message */
#define MAXANT      64                  /* max length of station name/antenna type */
#define MAXSOLBUF   256                 /* max number of solution buffer */
#define MAXOBSBUF   128                 /* max number of observation data buffer */
#define MAXNRPOS    16                  /* max number of reference positions */
#define MAXLEAPS    64                  /* max number of leap seconds table */
#define MAXGISLAYER 32                  /* max number of GIS data layers */
#define MAXRCVCMD   4096                /* max length of receiver commands */

#define CODE_NONE   0                   /* obs code: none or unknown */
#define CODE_L1C    1                   /* obs code: L1C/A,G1C/A,E1C (GPS,GLO,GAL,QZS,SBS) */
#define CODE_L1P    2                   /* obs code: L1P,G1P,B1P (GPS,GLO,BDS) */
#define CODE_L1W    3                   /* obs code: L1 Z-track (GPS) */
#define CODE_L1Y    4                   /* obs code: L1Y        (GPS) */
#define CODE_L1M    5                   /* obs code: L1M        (GPS) */
#define CODE_L1N    6                   /* obs code: L1codeless,B1codeless (GPS,BDS) */
#define CODE_L1S    7                   /* obs code: L1C(D)     (GPS,QZS) */
#define CODE_L1L    8                   /* obs code: L1C(P)     (GPS,QZS) */
#define CODE_L1E    9                   /* (not used) */
#define CODE_L1A    10                  /* obs code: E1A,B1A    (GAL,BDS) */
#define CODE_L1B    11                  /* obs code: E1B        (GAL) */
#define CODE_L1X    12                  /* obs code: E1B+C,L1C(D+P),B1D+P (GAL,QZS,BDS) */
#define CODE_L1Z    13                  /* obs code: E1A+B+C,L1S (GAL,QZS) */
#define CODE_L2C    14                  /* obs code: L2C/A,G1C/A (GPS,GLO) */
#define CODE_L2D    15                  /* obs code: L2 L1C/A-(P2-P1) (GPS) */
#define CODE_L2S    16                  /* obs code: L2C(M)     (GPS,QZS) */
#define CODE_L2L    17                  /* obs code: L2C(L)     (GPS,QZS) */
#define CODE_L2X    18                  /* obs code: L2C(M+L),B1_2I+Q (GPS,QZS,BDS) */
#define CODE_L2P    19                  /* obs code: L2P,G2P    (GPS,GLO) */
#define CODE_L2W    20                  /* obs code: L2 Z-track (GPS) */
#define CODE_L2Y    21                  /* obs code: L2Y        (GPS) */
#define CODE_L2M    22                  /* obs code: L2M        (GPS) */
#define CODE_L2N    23                  /* obs code: L2codeless (GPS) */
#define CODE_L5I    24                  /* obs code: L5I,E5aI   (GPS,GAL,QZS,SBS) */
#define CODE_L5Q    25                  /* obs code: L5Q,E5aQ   (GPS,GAL,QZS,SBS) */
#define CODE_L5X    26                  /* obs code: L5I+Q,E5aI+Q,L5B+C,B2aD+P (GPS,GAL,QZS,IRN,SBS,BDS) */
#define CODE_L7I    27                  /* obs code: E5bI,B2bI  (GAL,BDS) */
#define CODE_L7Q    28                  /* obs code: E5bQ,B2bQ  (GAL,BDS) */
#define CODE_L7X    29                  /* obs code: E5bI+Q,B2bI+Q (GAL,BDS) */
#define CODE_L6A    30                  /* obs code: E6A,B3A    (GAL,BDS) */
#define CODE_L6B    31                  /* obs code: E6B        (GAL) */
#define CODE_L6C    32                  /* obs code: E6C        (GAL) */
#define CODE_L6X    33                  /* obs code: E6B+C,LEXS+L,B3I+Q (GAL,QZS,BDS) */
#define CODE_L6Z    34                  /* obs code: E6A+B+C,L6D+E (GAL,QZS) */
#define CODE_L6S    35                  /* obs code: L6S        (QZS) */
#define CODE_L6L    36                  /* obs code: L6L        (QZS) */
#define CODE_L8I    37                  /* obs code: E5abI      (GAL) */
#define CODE_L8Q    38                  /* obs code: E5abQ      (GAL) */
#define CODE_L8X    39                  /* obs code: E5abI+Q,B2abD+P (GAL,BDS) */
#define CODE_L2I    40                  /* obs code: B1_2I      (BDS) */
#define CODE_L2Q    41                  /* obs code: B1_2Q      (BDS) */
#define CODE_L6I    42                  /* obs code: B3I        (BDS) */
#define CODE_L6Q    43                  /* obs code: B3Q        (BDS) */
#define CODE_L3I    44                  /* obs code: G3I        (GLO) */
#define CODE_L3Q    45                  /* obs code: G3Q        (GLO) */
#define CODE_L3X    46                  /* obs code: G3I+Q      (GLO) */
#define CODE_L1I    47                  /* obs code: B1I        (BDS) (obsolute) */
#define CODE_L1Q    48                  /* obs code: B1Q        (BDS) (obsolute) */
#define CODE_L5A    49                  /* obs code: L5A SPS    (IRN) */
#define CODE_L5B    50                  /* obs code: L5B RS(D)  (IRN) */
#define CODE_L5C    51                  /* obs code: L5C RS(P)  (IRN) */
#define CODE_L9A    52                  /* obs code: SA SPS     (IRN) */
#define CODE_L9B    53                  /* obs code: SB RS(D)   (IRN) */
#define CODE_L9C    54                  /* obs code: SC RS(P)   (IRN) */
#define CODE_L9X    55                  /* obs code: SB+C       (IRN) */
#define CODE_L1D    56                  /* obs code: B1D        (BDS) */
#define CODE_L5D    57                  /* obs code: L5D(L5S),B2aD (QZS,BDS) */
#define CODE_L5P    58                  /* obs code: L5P(L5S),B2aP (QZS,BDS) */
#define CODE_L5Z    59                  /* obs code: L5D+P(L5S) (QZS) */
#define CODE_L6E    60                  /* obs code: L6E        (QZS) */
#define CODE_L7D    61                  /* obs code: B2bD       (BDS) */
#define CODE_L7P    62                  /* obs code: B2bP       (BDS) */
#define CODE_L7Z    63                  /* obs code: B2bD+P     (BDS) */
#define CODE_L8D    64                  /* obs code: B2abD      (BDS) */
#define CODE_L8P    65                  /* obs code: B2abP      (BDS) */
#define CODE_L4A    66                  /* obs code: G1aL1OCd   (GLO) */
#define CODE_L4B    67                  /* obs code: G1aL1OCd   (GLO) */
#define CODE_L4X    68                  /* obs code: G1al1OCd+p (GLO) */
#define MAXCODE     68                  /* max number of obs code */

#ifndef EXTLEX
#define MAXRCVFMT    12                 /* max number of receiver format */
#else
#define MAXRCVFMT    16
#endif

#define LLI_SLIP    0x01                /* LLI: cycle-slip */
#define LLI_HALFC   0x02                /* LLI: half-cycle not resovled */
#define LLI_BOCTRK  0x04                /* LLI: boc tracking of mboc signal */
#define LLI_HALFA   0x40                /* LLI: half-cycle added */
#define LLI_HALFS   0x80                /* LLI: half-cycle subtracted */

#define P2_5        0.03125             /* 2^-5 */
#define P2_6        0.015625            /* 2^-6 */
#define P2_11       4.882812500000000E-04 /* 2^-11 */
#define P2_15       3.051757812500000E-05 /* 2^-15 */
#define P2_17       7.629394531250000E-06 /* 2^-17 */
#define P2_19       1.907348632812500E-06 /* 2^-19 */
#define P2_20       9.536743164062500E-07 /* 2^-20 */
#define P2_21       4.768371582031250E-07 /* 2^-21 */
#define P2_23       1.192092895507810E-07 /* 2^-23 */
#define P2_24       5.960464477539063E-08 /* 2^-24 */
#define P2_27       7.450580596923828E-09 /* 2^-27 */
#define P2_29       1.862645149230957E-09 /* 2^-29 */
#define P2_30       9.313225746154785E-10 /* 2^-30 */
#define P2_31       4.656612873077393E-10 /* 2^-31 */
#define P2_32       2.328306436538696E-10 /* 2^-32 */
#define P2_33       1.164153218269348E-10 /* 2^-33 */
#define P2_35       2.910383045673370E-11 /* 2^-35 */
#define P2_38       3.637978807091710E-12 /* 2^-38 */
#define P2_39       1.818989403545856E-12 /* 2^-39 */
#define P2_40       9.094947017729280E-13 /* 2^-40 */
#define P2_43       1.136868377216160E-13 /* 2^-43 */
#define P2_48       3.552713678800501E-15 /* 2^-48 */
#define P2_50       8.881784197001252E-16 /* 2^-50 */
#define P2_55       2.775557561562891E-17 /* 2^-55 */

//#ifdef WIN32
//#define thread_t    HANDLE
//#define lock_t      CRITICAL_SECTION
//#define initlock(f) InitializeCriticalSection(f)
//#define lock(f)     EnterCriticalSection(f)
//#define unlock(f)   LeaveCriticalSection(f)
//#define FILEPATHSEP '\\'
//#else
//#define thread_t    pthread_t
//#define lock_t      pthread_mutex_t
//#define initlock(f) pthread_mutex_init(f,NULL)
//#define lock(f)     pthread_mutex_lock(f)
//#define unlock(f)   pthread_mutex_unlock(f)
//#define FILEPATHSEP '/'
//#endif

/* type definitions ----------------------------------------------------------*/

typedef struct {        /* time struct */
    time_t time;        /* time (s) expressed by standard time_t */
    double sec;         /* fraction of second under 1 s */
} gtime_t;

typedef struct {        /* observation data record */
    gtime_t time;       /* receiver sampling time (GPST) */
    unsigned char sat,rcv; /* satellite/receiver number */
    unsigned char SNR [NFREQ+NEXOBS]; /* signal strength (0.25 dBHz) */
    unsigned char LLI [NFREQ+NEXOBS]; /* loss of lock indicator */
    int lockt[NFREQ+NEXOBS]; /* phase lock time */
    unsigned char code[NFREQ+NEXOBS]; /* code indicator (CODE_???) */
    double L[NFREQ+NEXOBS]; /* observation data carrier-phase (cycle) */
    double P[NFREQ+NEXOBS]; /* observation data pseudorange (m) */
    float  D[NFREQ+NEXOBS]; /* observation data doppler frequency (Hz) */
} obsd_t;

typedef struct {        /* observation data */
    int n,nmax;         /* number of obervation data/allocated */
    obsd_t *data;       /* observation data records */
} obs_t;

typedef struct {        /* earth rotation parameter data type */
    double mjd;         /* mjd (days) */
    double xp,yp;       /* pole offset (rad) */
    double xpr,ypr;     /* pole offset rate (rad/day) */
    double ut1_utc;     /* ut1-utc (s) */
    double lod;         /* length of day (s/day) */
} erpd_t;

typedef struct {        /* earth rotation parameter type */
    int n,nmax;         /* number and max number of data */
    erpd_t *data;       /* earth rotation parameter data */
} erp_t;

typedef struct {        /* antenna parameter type */
    int sat;            /* satellite number (0:receiver) */
    char type[MAXANT];  /* antenna type */
    char code[MAXANT];  /* serial number or satellite code */
    gtime_t ts,te;      /* valid time start and end */
    double off[NFREQ][ 3]; /* phase center offset e/n/u or x/y/z (m) */
    double var[NFREQ][19]; /* phase center variation (m) */
    /* el=90,85,...,0 or nadir=0,1,2,3,... (deg) */
} pcv_t;

typedef struct {        /* almanac type */
    int sat;            /* satellite number */
    int svh;            /* sv health (0:ok) */
    int svconf;         /* as and sv config */
    int week;           /* GPS/QZS: gps week, GAL: galileo week */
    gtime_t toa;        /* Toa */
    /* SV orbit parameters */
    double A,e,i0,OMG0,omg,M0,OMGd;
    double toas;        /* Toa (s) in week */
    double f0,f1;       /* SV clock parameters (af0,af1) */
} alm_t;

typedef struct {        /* GPS/QZS/GAL broadcast ephemeris type */
    int sat;            /* satellite number */
    int iode,iodc;      /* IODE,IODC */
    int sva;            /* SV accuracy (URA index) */
    int svh;            /* SV health (0:ok) */
    int week;           /* GPS/QZS: gps week, GAL: galileo week */
    int code;           /* GPS/QZS: code on L2 */
    /* GAL: data source defined as rinex 3.03 */
    /* BDS: data source (0:unknown,1:B1I,2:B1Q,3:B2I,4:B2Q,5:B3I,6:B3Q) */
    int flag;           /* GPS/QZS: L2 P data flag */
    /* BDS: nav type (0:unknown,1:IGSO/MEO,2:GEO) */
    gtime_t toe,toc,ttr; /* Toe,Toc,T_trans */
    /* SV orbit parameters */
    double A,e,i0,OMG0,omg,M0,deln,OMGd,idot;
    double crc,crs,cuc,cus,cic,cis;
    double toes;        /* Toe (s) in week */
    double fit;         /* fit interval (h) */
    double f0,f1,f2;    /* SV clock parameters (af0,af1,af2) */
  double tgd[6];      /* group delay parameters */
  /* GPS/QZS:tgd[0]=TGD */
  /* GAL:tgd[0]=BGD_E1E5a,tgd[1]=BGD_E1E5b */
  /* CMP:tgd[0]=TGD_B1I ,tgd[1]=TGD_B2I/B2b,tgd[2]=TGD_B1Cp */
  /*     tgd[3]=TGD_B2ap,tgd[4]=ISC_B1Cd   ,tgd[5]=ISC_B2ad */
    double Adot,ndot;   /* Adot,ndot for CNAV */
} eph_t;

typedef struct {        /* GLONASS broadcast ephemeris type */
    int sat;            /* satellite number */
    int iode;           /* IODE (0-6 bit of tb field) */
    int frq;            /* satellite frequency number */
    int svh,sva,age;    /* satellite health, accuracy, age of operation */
    gtime_t toe;        /* epoch of epherides (gpst) */
    gtime_t tof;        /* message frame time (gpst) */
    double pos[3];      /* satellite position (ecef) (m) */
    double vel[3];      /* satellite velocity (ecef) (m/s) */
    double acc[3];      /* satellite acceleration (ecef) (m/s^2) */
    double taun,gamn;   /* SV clock bias (s)/relative freq bias */
    double dtaun;       /* delay between L1 and L2 (s) */
} geph_t;

typedef struct {        /* precise ephemeris type */
    gtime_t time;       /* time (GPST) */
    int index;          /* ephemeris index for multiple files */
    double pos[MAXSAT][4]; /* satellite position/clock (ecef) (m|s) */
    float  std[MAXSAT][4]; /* satellite position/clock std (m|s) */
    double vel[MAXSAT][4]; /* satellite velocity/clk-rate (m/s|s/s) */
    float  vst[MAXSAT][4]; /* satellite velocity/clk-rate std (m/s|s/s) */
    float  cov[MAXSAT][3]; /* satellite position covariance (m^2) */
    float  vco[MAXSAT][3]; /* satellite velocity covariance (m^2) */
} peph_t;

typedef struct {        /* precise clock type */
    gtime_t time;       /* time (GPST) */
    int index;          /* clock index for multiple files */
    double clk[MAXSAT][1]; /* satellite clock (s) */
    float  std[MAXSAT][1]; /* satellite clock std (s) */
} pclk_t;

typedef struct {        /* SBAS ephemeris type */
    int sat;            /* satellite number */
    gtime_t t0;         /* reference epoch time (GPST) */
    gtime_t tof;        /* time of message frame (GPST) */
    int sva;            /* SV accuracy (URA index) */
    int svh;            /* SV health (0:ok) */
    double pos[3];      /* satellite position (m) (ecef) */
    double vel[3];      /* satellite velocity (m/s) (ecef) */
    double acc[3];      /* satellite acceleration (m/s^2) (ecef) */
    double af0,af1;     /* satellite clock-offset/drift (s,s/s) */
} seph_t;

typedef struct {        /* TEC grid type */
    gtime_t time;       /* epoch time (GPST) */
    int ndata[3];       /* TEC grid data size {nlat,nlon,nhgt} */
    double rb;          /* earth radius (km) */
    double lats[3];     /* latitude start/interval (deg) */
    double lons[3];     /* longitude start/interval (deg) */
    double hgts[3];     /* heights start/interval (km) */
    double *data;       /* TEC grid data (tecu) */
    float *rms;         /* RMS values (tecu) */
} tec_t;

typedef struct {        /* satellite fcb data type */
    gtime_t ts,te;      /* start/end time (GPST) */
    double bias[MAXSAT][3]; /* fcb value   (cyc) */
    double std [MAXSAT][3]; /* fcb std-dev (cyc) */
} fcbd_t;

typedef struct {        /* SBAS message type */
  int week,tow;       /* receiption time */
  uint8_t prn,rcv;    /* SBAS satellite PRN,receiver number */
  uint8_t msg[29];    /* SBAS message (226bit) padded by 0 */
} sbsmsg_t;

typedef struct {        /* SBAS messages type */
    int n,nmax;         /* number of SBAS messages/allocated */
    sbsmsg_t *msgs;     /* SBAS messages */
} sbs_t;

typedef struct {        /* SBAS fast correction type */
  gtime_t t0;         /* time of applicability (TOF) */
  double prc;         /* pseudorange correction (PRC) (m) */
  double rrc;         /* range-rate correction (RRC) (m/s) */
  double dt;          /* range-rate correction delta-time (s) */
  int iodf;           /* IODF (issue of date fast corr) */
  int16_t udre;       /* UDRE+1 */
  int16_t ai;         /* degradation factor indicator */
} sbsfcorr_t;

typedef struct {        /* SBAS long term satellite error correction type */
  gtime_t t0;         /* correction time */
  int iode;           /* IODE (issue of date ephemeris) */
  double dpos[3];     /* delta position (m) (ecef) */
  double dvel[3];     /* delta velocity (m/s) (ecef) */
  double daf0,daf1;   /* delta clock-offset/drift (s,s/s) */
} sbslcorr_t;

typedef struct {        /* SBAS satellite correction type */
  int sat;            /* satellite number */
  sbsfcorr_t fcorr;   /* fast correction */
  sbslcorr_t lcorr;   /* long term correction */
} sbssatp_t;

typedef struct {        /* SBAS satellite corrections type */
  int iodp;           /* IODP (issue of date mask) */
  int nsat;           /* number of satellites */
  int tlat;           /* system latency (s) */
  sbssatp_t sat[MAXSAT]; /* satellite correction */
} sbssat_t;

typedef struct {        /* SBAS ionospheric correction type */
  gtime_t t0;         /* correction time */
  int16_t lat,lon;    /* latitude/longitude (deg) */
  int16_t give;       /* GIVI+1 */
  float delay;        /* vertical delay estimate (m) */
} sbsigp_t;

typedef struct {        /* IGP band type */
  int16_t x;          /* longitude/latitude (deg) */
  const int16_t *y;   /* latitudes/longitudes (deg) */
  uint8_t bits;       /* IGP mask start bit */
  uint8_t bite;       /* IGP mask end bit */
} sbsigpband_t;

typedef struct {        /* SBAS ionospheric corrections type */
  int iodi;           /* IODI (issue of date ionos corr) */
  int nigp;           /* number of igps */
  sbsigp_t igp[MAXNIGP]; /* ionospheric correction */
} sbsion_t;

typedef struct {        /* DGPS/GNSS correction type */
  gtime_t t0;         /* correction time */
  double prc;         /* pseudorange correction (PRC) (m) */
  double rrc;         /* range rate correction (RRC) (m/s) */
  int iod;            /* issue of data (IOD) */
  double udre;        /* UDRE */
} dgps_t;

typedef struct {        /* SSR correction type */
  gtime_t t0[6];      /* epoch time (GPST) {eph,clk,hrclk,ura,bias,pbias} */
  double udi[6];      /* SSR update interval (s) */
  int iod[6];         /* iod ssr {eph,clk,hrclk,ura,bias,pbias} */
  int iode;           /* issue of data */
  int iodcrc;         /* issue of data crc for beidou/sbas */
  int ura;            /* URA indicator */
  int refd;           /* sat ref datum (0:ITRF,1:regional) */
  double deph [3];    /* delta orbit {radial,along,cross} (m) */
  double ddeph[3];    /* dot delta orbit {radial,along,cross} (m/s) */
  double dclk [3];    /* delta clock {c0,c1,c2} (m,m/s,m/s^2) */
  double hrclk;       /* high-rate clock corection (m) */
  float  cbias[MAXCODE]; /* code biases (m) */
  double pbias[MAXCODE]; /* phase biases (m) */
  float  stdpb[MAXCODE]; /* std-dev of phase biases (m) */
  double yaw_ang,yaw_rate; /* yaw angle and yaw rate (deg,deg/s) */
  uint8_t update;     /* update flag (0:no update,1:update) */
} ssr_t;

typedef struct {        /* navigation data type */
  int n,nmax;         /* number of broadcast ephemeris */
  int ng,ngmax;       /* number of glonass ephemeris */
  int ns,nsmax;       /* number of sbas ephemeris */
  int ne,nemax;       /* number of precise ephemeris */
  int nc,ncmax;       /* number of precise clock */
  int na,namax;       /* number of almanac data */
  int nt,ntmax;       /* number of tec grid data */
  eph_t *eph;         /* GPS/QZS/GAL/BDS/IRN ephemeris */
  geph_t *geph;       /* GLONASS ephemeris */
  seph_t *seph;       /* SBAS ephemeris */
  peph_t *peph;       /* precise ephemeris */
  pclk_t *pclk;       /* precise clock */
  alm_t *alm;         /* almanac data */
  tec_t *tec;         /* tec grid data */
  erp_t  erp;         /* earth rotation parameters */
  double utc_gps[8];  /* GPS delta-UTC parameters {A0,A1,Tot,WNt,dt_LS,WN_LSF,DN,dt_LSF} */
  double utc_glo[8];  /* GLONASS UTC time parameters {tau_C,tau_GPS} */
  double utc_gal[8];  /* Galileo UTC parameters */
  double utc_qzs[8];  /* QZS UTC parameters */
  double utc_cmp[8];  /* BeiDou UTC parameters */
  double utc_irn[9];  /* IRNSS UTC parameters {A0,A1,Tot,...,dt_LSF,A2} */
  double utc_sbs[4];  /* SBAS UTC parameters */
  double ion_gps[8];  /* GPS iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
  double ion_gal[4];  /* Galileo iono model parameters {ai0,ai1,ai2,0} */
  double ion_qzs[8];  /* QZSS iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
  double ion_cmp[8];  /* BeiDou iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
  double ion_irn[8];  /* IRNSS iono model parameters {a0,a1,a2,a3,b0,b1,b2,b3} */
  int glo_fcn[32];    /* GLONASS FCN + 8 */
  double cbias[MAXSAT][3]; /* satellite DCB (0:P1-P2,1:P1-C1,2:P2-C2) (m) */
  double rbias[MAXRCV][2][3]; /* receiver DCB (0:P1-P2,1:P1-C1,2:P2-C2) (m) */
  pcv_t pcvs[MAXSAT]; /* satellite antenna pcv */
  sbssat_t sbssat;    /* SBAS satellite corrections */
  sbsion_t sbsion[MAXBAND+1]; /* SBAS ionosphere corrections */
  dgps_t dgps[MAXSAT]; /* DGPS corrections */
  ssr_t ssr[MAXSAT];  /* SSR corrections */
} nav_t;

typedef struct {        /* station parameter type */
  char name   [MAXANT]; /* marker name */
  char marker [MAXANT]; /* marker number */
  char antdes [MAXANT]; /* antenna descriptor */
  char antsno [MAXANT]; /* antenna serial number */
  char rectype[MAXANT]; /* receiver type descriptor */
  char recver [MAXANT]; /* receiver firmware version */
  char recsno [MAXANT]; /* receiver serial number */
  int antsetup;       /* antenna setup id */
  int itrf;           /* ITRF realization year */
  int deltype;        /* antenna delta type (0:enu,1:xyz) */
  double pos[3];      /* station position (ecef) (m) */
  double del[3];      /* antenna position delta (e/n/u or x/y/z) (m) */
  double hgt;         /* antenna height (m) */
  int glo_cp_align;   /* GLONASS code-phase alignment (0:no,1:yes) */
  double glo_cp_bias[4]; /* GLONASS code-phase biases {1C,1P,2C,2P} (m) */
} sta_t;

typedef struct {        /* RTCM control struct type */
  int staid;          /* station id */
  int stah;           /* station health */
  int seqno;          /* sequence number for rtcm 2 or iods msm */
  int outtype;        /* output message type */
  gtime_t time;       /* message time */
  gtime_t time_s;     /* message start time */
  obs_t obs;          /* observation data (uncorrected) */
  nav_t nav;          /* satellite ephemerides */
  sta_t sta;          /* station parameters */
  dgps_t *dgps;       /* output of dgps corrections */
  ssr_t ssr[MAXSAT];  /* output of ssr corrections */
  char msg[128];      /* special message */
  char msgtype[256];  /* last message type */
  char msmtype[7][128]; /* msm signal types */
  int obsflag;        /* obs data complete flag (1:ok,0:not complete) */
  int ephsat;         /* input ephemeris satellite number */
  int ephset;         /* input ephemeris set (0-1) */
  double cp[MAXSAT][NFREQ+NEXOBS]; /* carrier-phase measurement */
  uint16_t lock[MAXSAT][NFREQ+NEXOBS]; /* lock time */
  uint16_t loss[MAXSAT][NFREQ+NEXOBS]; /* loss of lock count */
  gtime_t lltime[MAXSAT][NFREQ+NEXOBS]; /* last lock time */
  int nbyte;          /* number of bytes in message buffer */
  int nbit;           /* number of bits in word buffer */
  int len;            /* message length (bytes) */
  uint8_t buff[1200]; /* message buffer */
  uint32_t word;      /* word buffer for rtcm 2 */
  uint32_t nmsg2[100]; /* message count of RTCM 2 (1-99:1-99,0:other) */
  uint32_t nmsg3[400]; /* message count of RTCM 3 (1-299:1001-1299,300-329:4070-4099,0:ohter) */
  char opt[256];      /* RTCM dependent options */
} rtcm_t;

typedef struct {        /* option type */
  const char *name;   /* option name */
  int format;         /* option format (0:int,1:double,2:string,3:enum) */
  void *var;          /* pointer to option variable */
  const char *comment; /* option comment/enum labels/unit */
} opt_t;

typedef struct {        /* SNR mask type */
  int ena[2];         /* enable flag {rover,base} */
  double mask[NFREQ][9]; /* mask (dBHz) at 5,10,...85 deg */
} snrmask_t;

typedef struct {        /* processing options type */
  int mode;           /* positioning mode (PMODE_???) */
  int soltype;        /* solution type (0:forward,1:backward,2:combined) */
  int nf;             /* number of frequencies (1:L1,2:L1+L2,3:L1+L2+L5) */
  int navsys;         /* navigation system */
  double elmin;       /* elevation mask angle (rad) */
  snrmask_t snrmask;  /* SNR mask */
  int sateph;         /* satellite ephemeris/clock (EPHOPT_???) */
  int modear;         /* AR mode (0:off,1:continuous,2:instantaneous,3:fix and hold,4:ppp-ar) */
  int glomodear;      /* GLONASS AR mode (0:off,1:on,2:auto cal,3:ext cal) */
  int bdsmodear;      /* BeiDou AR mode (0:off,1:on) */
  int maxout;         /* obs outage count to reset bias */
  int minlock;        /* min lock count to fix ambiguity */
  int minfix;         /* min fix count to hold ambiguity */
  int armaxiter;      /* max iteration to resolve ambiguity */
  int ionoopt;        /* ionosphere option (IONOOPT_???) */
  int tropopt;        /* troposphere option (TROPOPT_???) */
  int dynamics;       /* dynamics model (0:none,1:velociy,2:accel) */
  int tidecorr;       /* earth tide correction (0:off,1:solid,2:solid+otl+pole) */
  int niter;          /* number of filter iteration */
  int codesmooth;     /* code smoothing window size (0:none) */
  int intpref;        /* interpolate reference obs (for post mission) */
  int sbascorr;       /* SBAS correction options */
  int sbassatsel;     /* SBAS satellite selection (0:all) */
  int rovpos;         /* rover position for fixed mode */
  int refpos;         /* base position for relative mode */
  /* (0:pos in prcopt,  1:average of single pos, */
  /*  2:read from file, 3:rinex header, 4:rtcm pos) */
  double eratio[NFREQ]; /* code/phase error ratio */
  double err[5];      /* measurement error factor */
  /* [0]:reserved */
  /* [1-3]:error factor a/b/c of phase (m) */
  /* [4]:doppler frequency (hz) */
  double std[3];      /* initial-state std [0]bias,[1]iono [2]trop */
  double prn[6];      /* process-noise std [0]bias,[1]iono [2]trop [3]acch [4]accv [5] pos */
  double sclkstab;    /* satellite clock stability (sec/sec) */
  double thresar[8];  /* AR validation threshold */
  double elmaskar;    /* elevation mask of AR for rising satellite (deg) */
  double elmaskhold;  /* elevation mask to hold ambiguity (deg) */
  double thresslip;   /* slip threshold of geometry-free phase (m) */
  double maxtdiff;    /* max difference of time (sec) */
  double maxinno;     /* reject threshold of innovation (m) */
  double maxgdop;     /* reject threshold of gdop */
  double baseline[2]; /* baseline length constraint {const,sigma} (m) */
  double ru[3];       /* rover position for fixed mode {x,y,z} (ecef) (m) */
  double rb[3];       /* base position for relative mode {x,y,z} (ecef) (m) */
  char anttype[2][MAXANT]; /* antenna types {rover,base} */
  double antdel[2][3]; /* antenna delta {{rov_e,rov_n,rov_u},{ref_e,ref_n,ref_u}} */
  pcv_t pcvr[2];      /* receiver antenna parameters {rov,base} */
  uint8_t exsats[MAXSAT]; /* excluded satellites (1:excluded,2:included) */
  int  maxaveep;      /* max averaging epoches */
  int  initrst;       /* initialize by restart */
  int  outsingle;     /* output single by dgps/float/fix/ppp outage */
  char rnxopt[2][256]; /* rinex options {rover,base} */
  int  posopt[6];     /* positioning options */
  int  syncsol;       /* solution sync mode (0:off,1:on) */
  double odisp[2][6*11]; /* ocean tide loading parameters {rov,base} */
  int  freqopt;       /* disable L2-AR */
  char pppopt[256];   /* ppp option */
} prcopt_t;

typedef struct {        /* solution options type */
  int posf;           /* solution format (SOLF_???) */
  int times;          /* time system (TIMES_???) */
  int timef;          /* time format (0:sssss.s,1:yyyy/mm/dd hh:mm:ss.s) */
  int timeu;          /* time digits under decimal point */
  int degf;           /* latitude/longitude format (0:ddd.ddd,1:ddd mm ss) */
  int outhead;        /* output header (0:no,1:yes) */
  int outopt;         /* output processing options (0:no,1:yes) */
  int outvel;         /* output velocity options (0:no,1:yes) */
  int datum;          /* datum (0:WGS84,1:Tokyo) */
  int height;         /* height (0:ellipsoidal,1:geodetic) */
  int geoid;          /* geoid model (0:EGM96,1:JGD2000) */
  int solstatic;      /* solution of static mode (0:all,1:single) */
  int sstat;          /* solution statistics level (0:off,1:states,2:residuals) */
  int trace;          /* debug trace level (0:off,1-5:debug) */
  double nmeaintv[2]; /* nmea output interval (s) (<0:no,0:all) */
  /* nmeaintv[0]:gprmc,gpgga,nmeaintv[1]:gpgsv */
  char sep[64];       /* field separator */
  char prog[64];      /* program name */
  double maxsolstd;   /* max std-dev for solution output (m) (0:all) */
} solopt_t;

typedef void fatalfunc_t(const char *); /* fatal callback function type */

/* global variables ----------------------------------------------------------*/
extern const double chisqr[];        /* chi-sqr(n) table (alpha=0.001) */
extern const double lam_carr[];      /* carrier wave length (m) {L1,L2,...} */
extern const prcopt_t prcopt_default; /* default positioning options */
extern const solopt_t solopt_default; /* default solution output options */
extern const sbsigpband_t igpband1[9][8]; /* SBAS IGP band 0-8 */
extern const sbsigpband_t igpband2[2][5]; /* SBAS IGP band 9-10 */
extern const char *formatstrs[];     /* stream format strings */
extern opt_t sysopts[];              /* system options table */

/* satellites, systems, codes functions --------------------------------------*/
EXPORT int  satno   (int sys, int prn);
EXPORT int  satsys  (int sat, int *prn);
EXPORT int  satid2no(const char *id);
EXPORT void satno2id(int sat, char *id);
EXPORT unsigned char obs2code(const char *obs, int *freq);
EXPORT std::string code2obs(uint8_t code);
EXPORT double code2freq(int sys, uint8_t code, int fcn);
EXPORT int  satexclude(int sat, double var, int svh, const prcopt_t *opt);
EXPORT int  testsnr(int base, int freq, double el, double snr,
                    const snrmask_t *mask);
EXPORT void setcodepri(int sys, int freq, const char *pri);
EXPORT int  getcodepri(int sys, unsigned char code, const char *opt);

/* matrix and vector functions -----------------------------------------------*/
EXPORT double *mat  (int n, int m);
EXPORT int    *imat (int n, int m);
EXPORT double *zeros(int n, int m);
EXPORT double *eye  (int n);
EXPORT double dot (const double *a, const double *b, int n);
EXPORT double norm(const double *a, int n);
EXPORT void cross3(const double *a, const double *b, double *c);
EXPORT int  normv3(const double *a, double *b);
EXPORT void matcpy(double *A, const double *B, int n, int m);
EXPORT void matmul(const char *tr, int n, int k, int m, double alpha,
                   const double *A, const double *B, double beta, double *C);
EXPORT int  matinv(double *A, int n);
EXPORT int  solve (const char *tr, const double *A, const double *Y, int n,
                   int m, double *X);
EXPORT int  lsq   (const double *A, const double *y, int n, int m, double *x,
                   double *Q);
EXPORT int  filter(double *x, double *P, const double *H, const double *v,
                   const double *R, int n, int m);
EXPORT int  smoother(const double *xf, const double *Qf, const double *xb,
                     const double *Qb, int n, double *xs, double *Qs);
EXPORT void matprint (const double *A, int n, int m, int p, int q);
EXPORT void matfprint(const double *A, int n, int m, int p, int q, FILE *fp);

EXPORT void add_fatal(fatalfunc_t *func);

/* time and string functions -------------------------------------------------*/
EXPORT double  str2num(const char *s, int i, int n);
EXPORT int     str2time(const char *s, int i, int n, gtime_t *t);
EXPORT void    time2str(gtime_t t, char *str, int n);
EXPORT gtime_t epoch2time(const double *ep);
EXPORT gtime_t epoch2time(std::vector<double> ep);
EXPORT void    time2epoch(gtime_t t, double *ep);
EXPORT gtime_t gpst2time(int week, double sec);
EXPORT double  time2gpst(gtime_t t, int *week);
EXPORT gtime_t gst2time(int week, double sec);
EXPORT double  time2gst(gtime_t t, int *week);
EXPORT gtime_t bdt2time(int week, double sec);
EXPORT double  time2bdt(gtime_t t, int *week);
EXPORT char    *time_str(gtime_t t, int n);

EXPORT gtime_t timeadd  (gtime_t t, double sec);
EXPORT double  timediff (gtime_t t1, gtime_t t2);
EXPORT gtime_t gpst2utc (gtime_t t);
EXPORT gtime_t utc2gpst (gtime_t t);
EXPORT gtime_t gpst2bdt (gtime_t t);
EXPORT gtime_t bdt2gpst (gtime_t t);
EXPORT gtime_t timeget  ();
EXPORT double  utc2gmst (gtime_t t, double ut1_utc);
EXPORT void sleepms(int ms);

/* coordinates transformation ------------------------------------------------*/
EXPORT void ecef2pos(const double *r, double *pos);
EXPORT void pos2ecef(const double *pos, double *r);
EXPORT void ecef2enu(const double *pos, const double *r, double *e);
EXPORT void enu2ecef(const double *pos, const double *e, double *r);
EXPORT void covenu  (const double *pos, const double *P, double *Q);
EXPORT void covecef (const double *pos, const double *Q, double *P);
EXPORT void xyz2enu (const double *pos, double *E);
EXPORT void eci2ecef(gtime_t tutc, const double *erpv, double *U, double *gmst);
EXPORT void deg2dms (double deg, double *dms, int ndec);
EXPORT double dms2deg(const double *dms);

/* input and output functions ------------------------------------------------*/
EXPORT void readpos(const char *file, const char *rcv, double *pos);
EXPORT int  sortobs(obs_t *obs);
EXPORT void uniqnav(nav_t *nav);
EXPORT int  screent(gtime_t time, gtime_t ts, gtime_t te, double tint);
EXPORT int  readnav(const char *file, nav_t *nav);
EXPORT int  savenav(const char *file, const nav_t *nav);
EXPORT void freeobs(obs_t *obs);
EXPORT void freenav(nav_t *nav, int opt);
EXPORT int  readblq(const char *file, const char *sta, double *odisp);
EXPORT int  readerp(const char *file, erp_t *erp);
EXPORT int  geterp (const erp_t *erp, gtime_t time, double *val);
/* positioning models --------------------------------------------------------*/
EXPORT double satwavelen(int sat, int frq, const nav_t *nav);
/* receiver raw data functions -----------------------------------------------*/
EXPORT void setbitu(uint8_t *buff, int pos, int len, uint32_t data);
EXPORT void setbits(uint8_t *buff, int pos, int len, int32_t data);
EXPORT uint32_t rtk_crc24q (const uint8_t *buff, int len);
EXPORT int model_phw(gtime_t time, int opt,
                     const double *rs, const double *rr, double &phw);
/* rtcm functions ------------------------------------------------------------*/
EXPORT void free_rtcm  (rtcm_t *rtcm);
EXPORT int gen_rtcm3   (rtcm_t *rtcm, int type, int sync);

//EXPORT int CreateRtcmMsg(double *date_time_gps,int n,const int *type,int m,
//        int fd, vector<double> sta_pos,vector<unsigned char> prn_recd,
//        vector<double> rtcm_pseR_L1,vector<double> rtcm_carrier_L1,
//        vector<double> rtcm_pseR_L2,vector<double> rtcm_carrier_L2,
//        vector<double> elev_rcd, vector<int> L2_rcd);//vector<vector<gtime_t>> &lltime_rcd

//#ifdef __cplusplus
//}
//#endif
#endif // WADGNSS_SERVER_RTKLIB_H
