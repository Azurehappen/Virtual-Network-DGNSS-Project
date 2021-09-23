/*------------------------------------------------------------------------------
* rtcm3e.c : rtcm ver.3 message encoder functions
*
*          Copyright (C) 2012-2019 by T.TAKASU, All rights reserved.
*
* references :
*     see rtcm.c
*
* version : $Revision:$ $Date:$
* history : 2012/12/05 1.0  new
*           2012/12/16 1.1  fix bug on ssr high rate clock correction
*           2012/12/24 1.2  fix bug on msm carrier-phase offset correction
*                           fix bug on SBAS sat id in 1001-1004
*                           fix bug on carrier-phase in 1001-1004,1009-1012
*           2012/12/28 1.3  fix bug on compass carrier wave length
*           2013/01/18 1.4  fix bug on ssr message generation
*           2013/05/11 1.5  change type of arg value of setbig()
*           2013/05/19 1.5  gpst -> bdt of time-tag in beidou msm message
*           2013/04/27 1.7  comply with rtcm 3.2 with amendment 1/2 (ref[15])
*                           delete MT 1046 according to ref [15]
*           2014/05/15 1.8  set NT field in MT 1020 glonass ephemeris
*           2014/12/06 1.9  support SBAS/BeiDou SSR messages (ref [16])
*                           fix bug on invalid staid in qzss ssr messages
*           2015/03/22 1.9  add handling of iodcrc for beidou/sbas ssr messages
*           2015/08/03 1.10 fix bug on wrong udint and iod in ssr 7.
*                           support rtcm ssr fcb message mt 2065-2069.
*           2015/09/07 1.11 add message count of MT 2000-2099
*           2015/10/21 1.12 add MT1046 support for IGS MGEX
*           2015/12/04 1.13 add MT63 beidou ephemeris (rtcm draft)
*                           fix bug on msm message generation of beidou
*                           fix bug on ssr 3 message generation (#321)
*           2016/06/12 1.14 fix bug on segmentation fault by generating msm1
*           2016/09/20 1.15 fix bug on MT1045 Galileo week rollover
*           2017/04/11 1.16 fix bug on gst-week in MT1045/1046
*           2018/10/10 1.17 merge changes for 2.4.2 p13
*                           change mt for ssr 7 phase biases
*           2019/05/10 1.21 save galileo E5b data to obs index 2
*-----------------------------------------------------------------------------*/
#include "rtklib.h"

/* constants and macros ------------------------------------------------------*/

#define PRUNIT_GPS  299792.458          /* rtcm 3 unit of gps pseudorange (m) */
#define PRUNIT_GLO  599584.916          /* rtcm 3 unit of glo pseudorange (m) */
#define RANGE_MS    (CLIGHT*0.001)      /* range in 1 ms */
#define P2_10       0.0009765625          /* 2^-10 */
#define P2_28       3.725290298461914E-09 /* 2^-28 */
#define P2_34       5.820766091346740E-11 /* 2^-34 */
#define P2_41       4.547473508864641E-13 /* 2^-41 */
#define P2_46       1.421085471520200E-14 /* 2^-46 */
#define P2_59       1.734723475976810E-18 /* 2^-59 */
#define P2_66       1.355252715606880E-20 /* 2^-66 */

#define ROUND(x)    ((int)floor((x)+0.5))
#define ROUND_U(x)  ((uint32_t)floor((x)+0.5))
#define MIN(x,y)    ((x)<(y)?(x):(y))

/* msm signal id table -------------------------------------------------------*/
//extern const char *msm_sig_gps[32];
//extern const char *msm_sig_glo[32];
//extern const char *msm_sig_gal[32];
//extern const char *msm_sig_qzs[32];
//extern const char *msm_sig_sbs[32];
//extern const char *msm_sig_cmp[32];
/* msm signal id table -------------------------------------------------------*/
const char *msm_sig_gps[32]={
    /* GPS: ref [17] table 3.5-91 */
    ""  ,"1C","1P","1W",""  ,""  ,""  ,"2C","2P","2W",""  ,""  , /*  1-12 */
    ""  ,""  ,"2S","2L","2X",""  ,""  ,""  ,""  ,"5I","5Q","5X", /* 13-24 */
    ""  ,""  ,""  ,""  ,""  ,"1S","1L","1X"                      /* 25-32 */
};
const char *msm_sig_glo[32]={
    /* GLONASS: ref [17] table 3.5-96 */
    ""  ,"1C","1P",""  ,""  ,""  ,""  ,"2C","2P",""  ,""  ,""  ,
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""
};
const char *msm_sig_gal[32]={
    /* Galileo: ref [17] table 3.5-99 */
    ""  ,"1C","1A","1B","1X","1Z",""  ,"6C","6A","6B","6X","6Z",
    ""  ,"7I","7Q","7X",""  ,"8I","8Q","8X",""  ,"5I","5Q","5X",
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""
};
const char *msm_sig_qzs[32]={
    /* QZSS: ref [17] table 3.5-105 */
    ""  ,"1C",""  ,""  ,""  ,""  ,""  ,""  ,"6S","6L","6X",""  ,
    ""  ,""  ,"2S","2L","2X",""  ,""  ,""  ,""  ,"5I","5Q","5X",
    ""  ,""  ,""  ,""  ,""  ,"1S","1L","1X"
};
const char *msm_sig_sbs[32]={
    /* SBAS: ref [17] table 3.5-102 */
    ""  ,"1C",""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,"5I","5Q","5X",
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""
};
const char *msm_sig_cmp[32]={
    /* BeiDou: ref [17] table 3.5-108 */
    ""  ,"2I","2Q","2X",""  ,""  ,""  ,"6I","6Q","6X",""  ,""  ,
    ""  ,"7I","7Q","7X",""  ,""  ,""  ,""  ,""  ,""  ,""  ,""  ,
    ""  ,""  ,""  ,""  ,""  ,""  ,""  ,""
};

/* set sign-magnitude bits ---------------------------------------------------*/
static void setbitg(uint8_t *buff, int pos, int len, int32_t value)
{
  setbitu(buff,pos,1,value<0?1:0);
  setbitu(buff,pos+1,len-1,value<0?-value:value);
}
/* set signed 38 bit field ---------------------------------------------------*/
static void set38bits(uint8_t *buff, int pos, double value)
{
  int word_h=(int)floor(value/64.0);
  uint32_t word_l=(uint32_t)(value-word_h*64.0);
  setbits(buff,pos  ,32,word_h);
  setbitu(buff,pos+32,6,word_l);
}
/* lock time -----------------------------------------------------------------*/
static int locktime(gtime_t time, gtime_t *lltime, uint8_t LLI)
{
  if (!lltime->time||(LLI&1)) *lltime=time;
  return (int)timediff(time,*lltime);
}
/* lock time indicator (ref [2] table 3.4-2) ---------------------------------*/
static int to_lock(int lock)
{
    if (lock<0  ) return 0;
    if (lock<24 ) return lock;
    if (lock<72 ) return (lock+24  )/2;
    if (lock<168) return (lock+120 )/4;
    if (lock<360) return (lock+408 )/8;
    if (lock<744) return (lock+1176)/16;
    if (lock<937) return (lock+3096)/32;
    return 127;
}
/* lock time in double -------------------------------------------------------*/
static double locktime_d(gtime_t time, gtime_t *lltime, uint8_t LLI)
{
  if (!lltime->time||(LLI&1)) *lltime=time;
  return timediff(time,*lltime);
}
/* msm lock time indicator (ref [17] table 3.5-74) ---------------------------*/
static int to_msm_lock(double lock)
{
  if (lock<0.032  ) return 0;
  if (lock<0.064  ) return 1;
  if (lock<0.128  ) return 2;
  if (lock<0.256  ) return 3;
  if (lock<0.512  ) return 4;
  if (lock<1.024  ) return 5;
  if (lock<2.048  ) return 6;
  if (lock<4.096  ) return 7;
  if (lock<8.192  ) return 8;
  if (lock<16.384 ) return 9;
  if (lock<32.768 ) return 10;
  if (lock<65.536 ) return 11;
  if (lock<131.072) return 12;
  if (lock<262.144) return 13;
  if (lock<524.288) return 14;
  return 15;
}
/* L1 code indicator gps -----------------------------------------------------*/
static int to_code1_gps(unsigned char code)
{
    switch (code) {
        case CODE_L1C: return 0; /* L1 C/A */
        case CODE_L1P:
        case CODE_L1W:
        case CODE_L1Y:
        case CODE_L1N: return 1; /* L1 P(Y) direct */
    }
    return 0;
}
/* L2 code indicator gps -----------------------------------------------------*/
static int to_code2_gps(unsigned char code)
{
    switch (code) {
        case CODE_L2C:
        case CODE_L2S:
        case CODE_L2L:
        case CODE_L2X: return 0; /* L2 C/A or L2C */
        case CODE_L2P:
        case CODE_L2Y: return 1; /* L2 P(Y) direct */
        case CODE_L2D: return 2; /* L2 P(Y) cross-correlated */
        case CODE_L2W:
        case CODE_L2N: return 3; /* L2 correlated P/Y */
    }
    return 0;
}
/* carrier-phase - pseudorange in cycle --------------------------------------*/
static double cp_pr(double cp, double pr_cyc)
{
    return fmod(cp-pr_cyc+750.0,1500.0)-750.0;
}

/* GLONASS frequency channel number in RTCM (FCN+7,-1:error) -----------------*/
static int fcn_glo(int sat, rtcm_t *rtcm)
{
  int prn;

  if (satsys(sat,&prn)!=SYS_GLO) {
    return -1;
  }
  if (rtcm->nav.geph[prn-1].sat==sat) {
    return rtcm->nav.geph[prn-1].frq+7;
  }
  if (rtcm->nav.glo_fcn[prn-1]>0) { /* fcn+8 (0: no data) */
    return rtcm->nav.glo_fcn[prn-1]-8+7;
  }
  return -1;
}

/* generate obs field data gps -----------------------------------------------*/
static void gen_obs_gps(rtcm_t *rtcm, const obsd_t *data, int *code1, int *pr1,
                        int *ppr1, int *lock1, int *amb, int *cnr1, int *code2,
                        int *pr21, int *ppr2, int *lock2, int *cnr2)
{
    double lam1,lam2,pr1c=0.0,ppr;
    int lt1,lt2;

    lam1=CLIGHT/FREQL1;
    lam2=CLIGHT/FREQL2;
    *pr1=*amb=0;
    if (ppr1) *ppr1=0xFFF80000; /* invalid values */
    if (pr21) *pr21=0xFFFFE000;
    if (ppr2) *ppr2=0xFFF80000;

    /* L1 peudorange */
    if (data->P[0]!=0.0&&data->code[0]) {
        *amb=(int)floor(data->P[0]/PRUNIT_GPS);
        *pr1=ROUND((data->P[0]-*amb*PRUNIT_GPS)/0.02);
        pr1c=*pr1*0.02+*amb*PRUNIT_GPS;
    }
    /* L1 phaserange - L1 pseudorange */
    if (data->P[0]!=0.0&&data->L[0]!=0.0&&data->code[0]) {
        ppr=cp_pr(data->L[0],pr1c/lam1);
        if (ppr1) *ppr1=ROUND(ppr*lam1/0.0005);
    }
    /* L2 -L1 pseudorange */
    if (data->P[0]!=0.0&&data->P[1]!=0.0&&data->code[0]&&data->code[1]&&
        fabs(data->P[1]-pr1c)<=163.82) {
        if (pr21) *pr21=ROUND((data->P[1]-pr1c)/0.02);
    }
    /* L2 phaserange - L1 pseudorange */
    if (data->P[0]!=0.0&&data->L[1]!=0.0&&data->code[0]&&data->code[1]) {
        ppr=cp_pr(data->L[1],pr1c/lam2);
        if (ppr2) *ppr2=ROUND(ppr*lam2/0.0005);
    }
    //lt1=locktime(data->time,rtcm->lltime[data->sat-1]  ,data->LLI[0]);
    //lt2=locktime(data->time,rtcm->lltime[data->sat-1]+1,data->LLI[1]);
    lt1 = data->lockt[0];
    lt2 = data->lockt[1];
    if (lock1) *lock1=to_lock(lt1);
    if (lock2) *lock2=to_lock(lt2);
    if (cnr1 ) *cnr1=data->SNR[0];
    if (cnr2 ) *cnr2=data->SNR[1];
    if (code1) *code1=to_code1_gps(data->code[0]);
    if (code2) *code2=to_code2_gps(data->code[1]);
    //printf("sat:%d,lock1:%d ",data->sat,*lock1);
    //printf("sat:%d,lock2:%d ",data->sat,*lock2); // Don't not uncomment, lock might be NULL, will cause seg fault
}
/* encode rtcm header --------------------------------------------------------*/
static int encode_head(int type, rtcm_t *rtcm, int sys, int sync, int nsat)
{
    double tow;
    int i=24,week,epoch;

    setbitu(rtcm->buff,i,12,type       ); i+=12; /* message no */
    setbitu(rtcm->buff,i,12,rtcm->staid); i+=12; /* ref station id */
    if (sys==SYS_GLO) {
        tow=time2gpst(timeadd(gpst2utc(rtcm->time),10800.0),&week);
        epoch=ROUND(fmod(tow,86400.0)/0.001);
        setbitu(rtcm->buff,i,27,epoch); i+=27; /* glonass epoch time */
    }
    else {
        tow=time2gpst(rtcm->time,&week);
        epoch=ROUND(tow/0.001);
        setbitu(rtcm->buff,i,30,epoch); i+=30; /* gps epoch time */
    }
    //printf("no of satellites: %d\n",nsat);
    setbitu(rtcm->buff,i, 1,sync); i+= 1; /* synchronous gnss flag */
    setbitu(rtcm->buff,i, 5,nsat); i+= 5; /* no of satellites */
    setbitu(rtcm->buff,i, 1,0   ); i+= 1; /* smoothing indicator */
    setbitu(rtcm->buff,i, 3,0   ); i+= 3; /* smoothing interval */
    return i;
}
/* encode type 1001: basic L1-only gps rtk observables -----------------------*/
static int encode_type1001(rtcm_t *rtcm, int sync)
{
    int i,j,nsat=0,sys,prn;
    int code1,pr1,ppr1,lock1,amb;

    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;
        nsat++;
    }
    /* encode header */
    i=encode_head(1001,rtcm,SYS_GPS,sync,nsat);

    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;

        if (sys==SYS_SBS) prn-=80; /* 40-58: sbas 120-138 */

        /* generate obs field data gps */
        gen_obs_gps(rtcm,rtcm->obs.data+j,&code1,&pr1,&ppr1,&lock1,&amb,NULL,
                    NULL,NULL,NULL,NULL,NULL);

        setbitu(rtcm->buff,i, 6,prn  ); i+= 6;
        setbitu(rtcm->buff,i, 1,code1); i+= 1;
        setbitu(rtcm->buff,i,24,pr1  ); i+=24;
        setbits(rtcm->buff,i,20,ppr1 ); i+=20;
        setbitu(rtcm->buff,i, 7,lock1); i+= 7;
    }
    rtcm->nbit=i;
    return 1;
}
/* encode type 1002: extended L1-only gps rtk observables --------------------*/
static int encode_type1002(rtcm_t *rtcm, int sync)
{
    int i,j,nsat=0,sys,prn;
    int code1,pr1,ppr1,lock1,amb,cnr1;
    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;
        nsat++;
    }
    /* encode header */
    i=encode_head(1002,rtcm,SYS_GPS,sync,nsat);

    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;

        if (sys==SYS_SBS) prn-=80; /* 40-58: sbas 120-138 */

        /* generate obs field data gps */
        gen_obs_gps(rtcm,rtcm->obs.data+j,&code1,&pr1,&ppr1,&lock1,&amb,&cnr1,
                    NULL,NULL,NULL,NULL,NULL);

        setbitu(rtcm->buff,i, 6,prn  ); i+= 6;
        setbitu(rtcm->buff,i, 1,code1); i+= 1;
        setbitu(rtcm->buff,i,24,pr1  ); i+=24;
        setbits(rtcm->buff,i,20,ppr1 ); i+=20;
        setbitu(rtcm->buff,i, 7,lock1); i+= 7;
        setbitu(rtcm->buff,i, 8,amb  ); i+= 8;
        setbitu(rtcm->buff,i, 8,cnr1 ); i+= 8;
    }
    rtcm->nbit=i;
    return 1;
}
/* encode type 1004: extended L1&L2 gps rtk observables ----------------------*/
static int encode_type1004(rtcm_t *rtcm, int sync)
{
    int i,j,nsat=0,sys,prn;
    int code1,pr1,ppr1,lock1,amb,cnr1,code2,pr21,ppr2,lock2,cnr2;

    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;
        nsat++;
    }
    /* encode header */
    i=encode_head(1004,rtcm,SYS_GPS,sync,nsat);

    for (j=0;j<rtcm->obs.n&&nsat<MAXOBS;j++) {
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        sys=satsys(rtcm->obs.data[j].sat,&prn);
        if (!(sys&(SYS_GPS|SYS_SBS))) continue;

        if (sys==SYS_SBS) prn-=80; /* 40-58: sbas 120-138 */

        /* generate obs field data gps */
        gen_obs_gps(rtcm,rtcm->obs.data+j,&code1,&pr1,&ppr1,&lock1,&amb,
                    &cnr1,&code2,&pr21,&ppr2,&lock2,&cnr2);

        setbitu(rtcm->buff,i, 6,prn  ); i+= 6;
        setbitu(rtcm->buff,i, 1,code1); i+= 1;
        setbitu(rtcm->buff,i,24,pr1  ); i+=24;
        setbits(rtcm->buff,i,20,ppr1 ); i+=20;
        setbitu(rtcm->buff,i, 7,lock1); i+= 7;
        setbitu(rtcm->buff,i, 8,amb  ); i+= 8;
        setbitu(rtcm->buff,i, 8,cnr1 ); i+= 8;
        setbitu(rtcm->buff,i, 2,code2); i+= 2;
        setbits(rtcm->buff,i,14,pr21 ); i+=14;
        setbits(rtcm->buff,i,20,ppr2 ); i+=20;
        setbitu(rtcm->buff,i, 7,lock2); i+= 7;
        setbitu(rtcm->buff,i, 8,cnr2 ); i+= 8;
    }
    rtcm->nbit=i;
    return 1;
}
/* encode type 1005: stationary rtk reference station arp --------------------*/
static int encode_type1005(rtcm_t *rtcm, int sync)
{
    double *p=rtcm->sta.pos;
    int i=24;
    //trace(3,"encode_type1005: sync=%d\n",sync);
    //printf("p:%.5f,%.5f,%.5f\n",p[0],p[1],p[2]);
    setbitu(rtcm->buff,i,12,1005       ); i+=12; /* message no */
    setbitu(rtcm->buff,i,12,rtcm->staid); i+=12; /* ref station id */
    setbitu(rtcm->buff,i, 6,0          ); i+= 6; /* itrf realization year */
    setbitu(rtcm->buff,i, 1,1          ); i+= 1; /* gps indicator */
    setbitu(rtcm->buff,i, 1,1          ); i+= 1; /* glonass indicator */
    setbitu(rtcm->buff,i, 1,0          ); i+= 1; /* galileo indicator */
    setbitu(rtcm->buff,i, 1,0          ); i+= 1; /* ref station indicator */
    set38bits(rtcm->buff,i,p[0]/0.0001 ); i+=38; /* antenna ref point ecef-x */
    setbitu(rtcm->buff,i, 1,1          ); i+= 1; /* oscillator indicator */
    setbitu(rtcm->buff,i, 1,0          ); i+= 1; /* reserved */
    set38bits(rtcm->buff,i,p[1]/0.0001 ); i+=38; /* antenna ref point ecef-y */
    setbitu(rtcm->buff,i, 2,0          ); i+= 2; /* quarter cycle indicator */
    set38bits(rtcm->buff,i,p[2]/0.0001 ); i+=38; /* antenna ref point ecef-z */
    rtcm->nbit=i;
    return 1;
}

/* satellite no to msm satellite id ------------------------------------------*/
static int to_satid(int sys, int sat)
{
  int prn;

  if (satsys(sat,&prn)!=sys) return 0;

  //if      (sys==SYS_QZS) prn-=MINPRNQZS-1;
  //else if (sys==SYS_SBS) prn-=MINPRNSBS-1;

  return prn;
}

/* observation code to msm signal id -----------------------------------------*/
static int to_sigid(int sys, uint8_t code)
{
  const char **msm_sig;
  std::string sig;
  int i;

  /* signal conversion for undefined signal by rtcm */
  if (sys==SYS_GPS) {
    if      (code==CODE_L1Y) code=CODE_L1P;
    else if (code==CODE_L1M) code=CODE_L1P;
    else if (code==CODE_L1N) code=CODE_L1P;
    else if (code==CODE_L2D) code=CODE_L2P;
    else if (code==CODE_L2Y) code=CODE_L2P;
    else if (code==CODE_L2M) code=CODE_L2P;
    else if (code==CODE_L2N) code=CODE_L2P;
  }
  sig = code2obs(code);
  if (sig.empty()) return 0;

  switch (sys) {
  case SYS_GPS: msm_sig=msm_sig_gps; break;
  case SYS_GLO: msm_sig=msm_sig_glo; break;
  case SYS_GAL: msm_sig=msm_sig_gal; break;
  case SYS_QZS: msm_sig=msm_sig_qzs; break;
  case SYS_SBS: msm_sig=msm_sig_sbs; break;
  case SYS_CMP: msm_sig=msm_sig_cmp; break;
  default: return 0;
  }
  for (i=0;i<32;i++) {
    if (!strcmp(sig.c_str(),msm_sig[i])) return i+1;
  }
  return 0;
}

/* generate msm satellite, signal and cell index -----------------------------*/
static void gen_msm_index(rtcm_t *rtcm, int sys, int *nsat, int *nsig,
                          int *ncell, uint8_t *sat_ind, uint8_t *sig_ind,
                          uint8_t *cell_ind)
{
  int i,j,sat,sig,cell;

  *nsat=*nsig=*ncell=0;

  /* generate satellite and signal index */
  for (i=0;i<rtcm->obs.n;i++) {
    if (!(sat=to_satid(sys,rtcm->obs.data[i].sat))) continue;

    for (j=0;j<NFREQ+NEXOBS;j++) {
      if (!(sig=to_sigid(sys,rtcm->obs.data[i].code[j]))) continue;

      sat_ind[sat-1]=sig_ind[sig-1]=1;
    }
  }
  for (i=0;i<64;i++) {
    if (sat_ind[i]) sat_ind[i]=++(*nsat);
  }
  for (i=0;i<32;i++) {
    if (sig_ind[i]) sig_ind[i]=++(*nsig);
  }
  /* generate cell index */
  for (i=0;i<rtcm->obs.n;i++) {
    if (!(sat=to_satid(sys,rtcm->obs.data[i].sat))) continue;

    for (j=0;j<NFREQ+NEXOBS;j++) {
      if (!(sig=to_sigid(sys,rtcm->obs.data[i].code[j]))) continue;

      cell=sig_ind[sig-1]-1+(sat_ind[sat-1]-1)*(*nsig);
      cell_ind[cell]=1;
    }
  }
  for (i=0;i<*nsat*(*nsig);i++) {
    if (cell_ind[i]&&*ncell<64) cell_ind[i]=++(*ncell);
  }
}

/* generate msm satellite data fields ----------------------------------------*/
static void gen_msm_sat(rtcm_t *rtcm, int sys, int nsat, const uint8_t *sat_ind,
                        double *rrng, double *rrate, uint8_t *info)
{
  obsd_t *data;
  double freq;
  int i,j,k,sat,sig,fcn;

  for (i=0;i<64;i++) rrng[i]=rrate[i]=0.0;

  for (i=0;i<rtcm->obs.n;i++) {
    data=rtcm->obs.data+i;
    fcn=fcn_glo(data->sat,rtcm); /* fcn+7 */
    if (!(sat=to_satid(sys,data->sat))) continue;

    for (j=0;j<NFREQ+NEXOBS;j++) {
      if (!(sig=to_sigid(sys,data->code[j]))) continue;
      k=sat_ind[sat-1]-1;
      freq=code2freq(sys,data->code[j],fcn-7);

      /* rough range (ms) and rough phase-range-rate (m/s) */
      if (rrng[k]==0.0&&data->P[j]!=0.0) {
        rrng[k]=ROUND( data->P[j]/RANGE_MS/P2_10)*RANGE_MS*P2_10;
      }
      if (rrate[k]==0.0&&data->D[j]!=0.0&&freq>0.0) {
        rrate[k]=ROUND(-data->D[j]*CLIGHT/freq)*1.0;
      }

      /* extended satellite info */
      if (info) info[k]=sys!=SYS_GLO?0:(fcn<0?15:fcn);
    }
  }
}
/* generate msm signal data fields -------------------------------------------*/
static void gen_msm_sig(rtcm_t *rtcm, int sys, int nsat, int nsig, int ncell,
                        const uint8_t *sat_ind,
                        const uint8_t *sig_ind,
                        const uint8_t *cell_ind, const double *rrng,
                        const double *rrate, double *psrng, double *phrng,
                        double *rate, double *lock, uint8_t *half,
                        float *cnr)
{
  obsd_t *data;
  double freq,lambda,psrng_s,phrng_s,rate_s,lt;
  int i,j,k,sat,sig,fcn,cell,LLI;

  for (i=0;i<ncell;i++) {
    if (psrng) psrng[i]=0.0;
    if (phrng) phrng[i]=0.0;
    if (rate ) rate [i]=0.0;
  }
  for (i=0;i<rtcm->obs.n;i++) {
    data=rtcm->obs.data+i;

    if (!(sat=to_satid(sys,data->sat))) continue;

    for (j=0;j<NFREQ+NEXOBS;j++) {
      if (!(sig=to_sigid(sys,data->code[j]))) continue;
      k=sat_ind[sat-1]-1;
      if ((cell=cell_ind[sig_ind[sig-1]-1+k*nsig])>=64) continue;

      freq=code2freq(sys,data->code[j],fcn-7);
      lambda=freq==0.0?0.0:CLIGHT/freq;
      psrng_s=data->P[j]==0.0?0.0:data->P[j]-rrng[k];
      phrng_s=data->L[j]==0.0||lambda<=0.0?0.0: data->L[j]*lambda-rrng [k];
      rate_s =data->D[j]==0.0||lambda<=0.0?0.0:-data->D[j]*lambda-rrate[k];

      /* subtract phase - psudorange integer cycle offset */
      LLI=data->LLI[j];
      if ((LLI&1)||fabs(phrng_s-rtcm->cp[data->sat-1][j])>1171.0) {
        rtcm->cp[data->sat-1][j]=ROUND(phrng_s/lambda)*lambda;
        LLI|=1;
      }
      phrng_s-=rtcm->cp[data->sat-1][j];

      lt=locktime_d(data->time,rtcm->lltime[data->sat-1]+j,LLI);

      if (psrng&&psrng_s!=0.0) psrng[cell-1]=psrng_s;
      if (phrng&&phrng_s!=0.0) phrng[cell-1]=phrng_s;
      if (rate &&rate_s !=0.0) rate [cell-1]=rate_s;
      if (lock) lock[cell-1]=lt;
      if (half) half[cell-1]=(data->LLI[j]&2)?1:0;
      if (cnr ) cnr [cell-1]=(float)(data->SNR[j]*0.25);
    }
  }
}

/* encode msm header ---------------------------------------------------------*/
static int encode_msm_head(int type, rtcm_t *rtcm, int sys, int sync, int *nsat,
                           int *ncell, double *rrng, double *rrate,
                           uint8_t *info, double *psrng, double *phrng,
                           double *rate, double *lock, uint8_t *half,
                           float *cnr)
{
  double tow;
  uint8_t sat_ind[64]={0},sig_ind[32]={0},cell_ind[32*64]={0};
  uint32_t dow,epoch;
  int i=24,j,nsig=0;

  switch (sys) {
  case SYS_GPS: type+=1070; break;
  case SYS_GLO: type+=1080; break;
  case SYS_GAL: type+=1090; break;
  case SYS_QZS: type+=1110; break;
  case SYS_SBS: type+=1100; break;
  case SYS_CMP: type+=1120; break;
  default: return 0;
  }
  /* generate msm satellite, signal and cell index */
  gen_msm_index(rtcm,sys,nsat,&nsig,ncell,sat_ind,sig_ind,cell_ind);

  if (sys==SYS_GLO) {
    /* glonass time (dow + tod-ms) */
    tow=time2gpst(timeadd(gpst2utc(rtcm->time),10800.0),NULL);
    dow=(uint32_t)(tow/86400.0);
    epoch=(dow<<27)+ROUND_U(fmod(tow,86400.0)*1E3);
  }
  else if (sys==SYS_CMP) {
    /* beidou time (tow-ms) */
    epoch=ROUND_U(time2gpst(gpst2bdt(rtcm->time),NULL)*1E3);
  }
  else {
    /* gps, qzs and galileo time (tow-ms) */
    epoch=ROUND_U(time2gpst(rtcm->time,NULL)*1E3);
  }
  /* encode msm header (ref [15] table 3.5-78) */
  setbitu(rtcm->buff,i,12,type       ); i+=12; /* message number */
  setbitu(rtcm->buff,i,12,rtcm->staid); i+=12; /* reference station id */
  setbitu(rtcm->buff,i,30,epoch      ); i+=30; /* epoch time */
  setbitu(rtcm->buff,i, 1,sync       ); i+= 1; /* multiple message bit */
  setbitu(rtcm->buff,i, 3,rtcm->seqno); i+= 3; /* issue of data station */
  setbitu(rtcm->buff,i, 7,0          ); i+= 7; /* reserved */
  setbitu(rtcm->buff,i, 2,0          ); i+= 2; /* clock streering indicator */
  setbitu(rtcm->buff,i, 2,0          ); i+= 2; /* external clock indicator */
  setbitu(rtcm->buff,i, 1,0          ); i+= 1; /* smoothing indicator */
  setbitu(rtcm->buff,i, 3,0          ); i+= 3; /* smoothing interval */

  /* satellite mask */
  for (j=0;j<64;j++) {
    setbitu(rtcm->buff,i,1,sat_ind[j]?1:0); i+=1;
  }
  /* signal mask */
  for (j=0;j<32;j++) {
    setbitu(rtcm->buff,i,1,sig_ind[j]?1:0); i+=1;
  }
  /* cell mask */
  for (j=0;j<*nsat*nsig&&j<64;j++) {
    setbitu(rtcm->buff,i,1,cell_ind[j]?1:0); i+=1;
  }
  /* generate msm satellite data fields */
  gen_msm_sat(rtcm,sys,*nsat,sat_ind,rrng,rrate,info);

  /* generate msm signal data fields */
  gen_msm_sig(rtcm,sys,*nsat,nsig,*ncell,sat_ind,sig_ind,cell_ind,rrng,rrate,
              psrng,phrng,rate,lock,half,cnr);

  return i;
}
/* encode rough range integer ms ---------------------------------------------*/
static int encode_msm_int_rrng(rtcm_t *rtcm, int i, const double *rrng,
                               int nsat)
{
  uint32_t int_ms;
  int j;

  for (j=0;j<nsat;j++) {
    if (rrng[j]==0.0) {
      int_ms=255;
    }
    else if (rrng[j]<0.0||rrng[j]>RANGE_MS*255.0) {
      //trace(2,"msm rough range overflow %s rrng=%.3f\n",
      //      time_str(rtcm->time,0),rrng[j]);
      int_ms=255;
    }
    else {
      int_ms=ROUND_U(rrng[j]/RANGE_MS/P2_10)>>10;
    }
    setbitu(rtcm->buff,i,8,int_ms); i+=8;
  }
  return i;
}
/* encode rough range modulo 1 ms --------------------------------------------*/
static int encode_msm_mod_rrng(rtcm_t *rtcm, int i, const double *rrng,
                               int nsat)
{
  uint32_t mod_ms;
  int j;

  for (j=0;j<nsat;j++) {
    if (rrng[j]<=0.0||rrng[j]>RANGE_MS*255.0) {
      mod_ms=0;
    }
    else {
      mod_ms=ROUND_U(rrng[j]/RANGE_MS/P2_10)&0x3FFu;
    }
    setbitu(rtcm->buff,i,10,mod_ms); i+=10;
  }
  return i;
}
/* encode lock-time indicator ------------------------------------------------*/
static int encode_msm_lock(rtcm_t *rtcm, int i, const double *lock, int ncell)
{
  int j,lock_val;

  for (j=0;j<ncell;j++) {
    //lock_val=to_msm_lock(lock[j]);
    lock_val = 0; // Lock time max
    setbitu(rtcm->buff,i,4,lock_val); i+=4;
  }
  return i;
}
/* encode fine pseudorange ---------------------------------------------------*/
static int encode_msm_psrng(rtcm_t *rtcm, int i, const double *psrng, int ncell)
{
  int j,psrng_val;

  for (j=0;j<ncell;j++) {
    if (psrng[j]==0.0) {
      psrng_val=-16384;
    }
    else if (fabs(psrng[j])>292.7) {
      //trace(2,"msm fine pseudorange overflow %s psrng=%.3f\n",
      //      time_str(rtcm->time,0),psrng[j]);
      psrng_val=-16384;
    }
    else {
      psrng_val=ROUND(psrng[j]/RANGE_MS/P2_24);
    }
    setbits(rtcm->buff,i,15,psrng_val); i+=15;
  }
  return i;
}
/* encode fine phase-range ---------------------------------------------------*/
static int encode_msm_phrng(rtcm_t *rtcm, int i, const double *phrng, int ncell)
{
  int j,phrng_val;

  for (j=0;j<ncell;j++) {
    if (phrng[j]==0.0) {
      phrng_val=-2097152;
    }
    else if (fabs(phrng[j])>1171.0) {
      //trace(2,"msm fine phase-range overflow %s phrng=%.3f\n",
      //      time_str(rtcm->time,0),phrng[j]);
      phrng_val=-2097152;
    }
    else {
      phrng_val=ROUND(phrng[j]/RANGE_MS/P2_29);
    }
    setbits(rtcm->buff,i,22,phrng_val); i+=22;
  }
  return i;
}
/* encode half-cycle-ambiguity indicator -------------------------------------*/
static int encode_msm_half_amb(rtcm_t *rtcm, int i, const uint8_t *half,
                               int ncell)
{
  int j;

  for (j=0;j<ncell;j++) {
    setbitu(rtcm->buff,i,1,half[j]); i+=1;
  }
  return i;
}
/* encode signal cnr ---------------------------------------------------------*/
static int encode_msm_cnr(rtcm_t *rtcm, int i, const float *cnr, int ncell)
{
  int j,cnr_val;

  for (j=0;j<ncell;j++) {
    cnr_val=ROUND(cnr[j]/1.0);
    setbitu(rtcm->buff,i,6,cnr_val); i+=6;
  }
  return i;
}

/* encode MSM 1: compact pseudorange -----------------------------------------*/
static int encode_msm1(rtcm_t *rtcm, int sys, int sync)
{
  double rrng[64],rrate[64],psrng[64];
  int i,nsat,ncell;

  /* encode msm header */
  if (!(i=encode_msm_head(1,rtcm,sys,sync,&nsat,&ncell,rrng,rrate,NULL,psrng,
                          NULL,NULL,NULL,NULL,NULL))) {
    return 0;
  }
  /* encode msm satellite data */
  i=encode_msm_mod_rrng(rtcm,i,rrng ,nsat ); /* rough range modulo 1 ms */

  /* encode msm signal data */
  i=encode_msm_psrng   (rtcm,i,psrng,ncell); /* fine pseudorange */

  rtcm->nbit=i;
  return 1;
}

/* encode MSM 2: compact phaserange ------------------------------------------*/
static int encode_msm2(rtcm_t *rtcm, int sys, int sync)
{
  double rrng[64],rrate[64],phrng[64],lock[64];
  uint8_t half[64];
  int i,nsat,ncell;

  /* encode msm header */
  if (!(i=encode_msm_head(2,rtcm,sys,sync,&nsat,&ncell,rrng,rrate,NULL,NULL,
                          phrng,NULL,lock,half,NULL))) {
    return 0;
  }
  /* encode msm satellite data */
  i=encode_msm_mod_rrng(rtcm,i,rrng ,nsat ); /* rough range modulo 1 ms */

  /* encode msm signal data */
  i=encode_msm_phrng   (rtcm,i,phrng,ncell); /* fine phase-range */
  i=encode_msm_lock    (rtcm,i,lock ,ncell); /* lock-time indicator */
  i=encode_msm_half_amb(rtcm,i,half ,ncell); /* half-cycle-amb indicator */

  rtcm->nbit=i;
  return 1;
}

/* encode msm 4: full pseudorange and phaserange plus cnr --------------------*/
static int encode_msm4(rtcm_t *rtcm, int sys, int sync)
{
  double rrng[64],rrate[64],psrng[64],phrng[64],lock[64];
  float cnr[64];
  uint8_t half[64];
  int i,nsat,ncell;

  //trace(3,"encode_msm4: sys=%d sync=%d\n",sys,sync);

  /* encode msm header */
  if (!(i=encode_msm_head(4,rtcm,sys,sync,&nsat,&ncell,rrng,rrate,NULL,psrng,
                          phrng,NULL,lock,half,cnr))) {
    return 0;
  }
  /* encode msm satellite data */
  i=encode_msm_int_rrng(rtcm,i,rrng ,nsat ); /* rough range integer ms */
  i=encode_msm_mod_rrng(rtcm,i,rrng ,nsat ); /* rough range modulo 1 ms */

  /* encode msm signal data */
  i=encode_msm_psrng   (rtcm,i,psrng,ncell); /* fine pseudorange */
  i=encode_msm_phrng   (rtcm,i,phrng,ncell); /* fine phase-range */
  i=encode_msm_lock    (rtcm,i,lock ,ncell); /* lock-time indicator */
  i=encode_msm_half_amb(rtcm,i,half ,ncell); /* half-cycle-amb indicator */
  i=encode_msm_cnr     (rtcm,i,cnr  ,ncell); /* signal cnr */
  rtcm->nbit=i;
  return 1;
}



/* encode rtcm ver.3 message -------------------------------------------------*/
extern int encode_rtcm3(rtcm_t *rtcm, int type, int sync)
{
    int ret=0;

    switch (type) {
      case 1001: ret=encode_type1001(rtcm,sync);     break;
      case 1002: ret=encode_type1002(rtcm,sync);     break;
      case 1004: ret=encode_type1004(rtcm,sync);     break;
      case 1005: ret=encode_type1005(rtcm,sync);     break;
      case 1071: ret=encode_msm1(rtcm,SYS_GPS,sync); break; // GPS MSM1
      case 1072: ret=encode_msm2(rtcm,SYS_GPS,sync); break; // GPS MSM1
      case 1074: ret=encode_msm4(rtcm,SYS_GPS,sync); break; // GPS MSM4
      case 1091: ret=encode_msm1(rtcm,SYS_GAL,sync); break; // GAL MSM1
      case 1092: ret=encode_msm2(rtcm,SYS_GAL,sync); break; // GAL MSM1
      case 1094: ret=encode_msm4(rtcm,SYS_GAL,sync); break; // GAL MSM4
      case 1121: ret=encode_msm1(rtcm,SYS_CMP,sync); break; // BDS MSM1
      case 1122: ret=encode_msm2(rtcm,SYS_CMP,sync); break; // BDS MSM1
      case 1124: ret=encode_msm4(rtcm,SYS_CMP,sync); break; // BDS MSM4
    }
    if (ret>0) {
        type-=1000;
        if      (   1<=type&&type<= 299) rtcm->nmsg3[type    ]++; /* 1001-1299 */
        else if (1000<=type&&type<=1099) rtcm->nmsg3[type-700]++; /* 2000-2099 */
        else rtcm->nmsg3[0]++;
    }
    return ret;
}


