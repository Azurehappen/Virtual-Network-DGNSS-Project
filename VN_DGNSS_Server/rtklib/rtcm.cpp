/*------------------------------------------------------------------------------
* rtcm.c : rtcm functions
*
*          Copyright (C) 2009-2018 by T.TAKASU, All rights reserved.
*
* references :
*     [1] RTCM Recommended Standards for Differential GNSS (Global Navigation
*         Satellite Systems) Service version 2.3, August 20, 2001
*     [2] RTCM Standard 10403.1 for Differential GNSS (Global Navigation
*         Satellite Systems) Services - Version 3, Octobar 27, 2006
*     [3] RTCM 10403.1-Amendment 3, Amendment 3 to RTCM Standard 10403.1
*     [4] RTCM Paper, April 12, 2010, Proposed SSR Messages for SV Orbit Clock,
*         Code Biases, URA
*     [5] RTCM Paper 012-2009-SC104-528, January 28, 2009 (previous ver of [4])
*     [6] RTCM Paper 012-2009-SC104-582, February 2, 2010 (previous ver of [4])
*     [7] RTCM Standard 10403.1 - Amendment 5, Differential GNSS (Global
*         Navigation Satellite Systems) Services - version 3, July 1, 2011
*     [8] RTCM Paper 019-2012-SC104-689 (draft Galileo ephmeris messages)
*     [9] RTCM Paper 163-2012-SC104-725 (draft QZSS ephemeris message)
*     [10] RTCM Paper 059-2011-SC104-635 (draft Galileo and QZSS ssr messages)
*     [11] RTCM Paper 034-2012-SC104-693 (draft multiple signal messages)
*     [12] RTCM Paper 133-2012-SC104-709 (draft QZSS MSM messages)
*     [13] RTCM Paper 122-2012-SC104-707.r1 (draft MSM messages)
*     [14] RTCM Standard 10403.2, Differential GNSS (Global Navigation Satellite
*          Systems) Services - version 3, February 1, 2013
*     [15] RTCM Standard 10403.2, Differential GNSS (Global Navigation Satellite
*          Systems) Services - version 3, with amendment 1/2, november 7, 2013
*     [16] Proposal of new RTCM SSR Messages (ssr_1_gal_qzss_sbas_dbs_v05)
*          2014/04/17
*     [17] RTCM Standard 10403.2, Differential GNSS (Global Navigation Satellite
*          Systems) Services - version 3, October 7, 2016
*
* version : $Revision:$ $Date:$
* history : 2009/04/10 1.0  new
*           2009/06/29 1.1  support type 1009-1012 to get synchronous-gnss-flag
*           2009/12/04 1.2  support type 1010,1012,1020
*           2010/07/15 1.3  support type 1057-1068 for ssr corrections
*                           support type 1007,1008,1033 for antenna info
*           2010/09/08 1.4  fix problem of ephemeris and ssr sequence upset
*                           (2.4.0_p8)
*           2012/05/11 1.5  comply with RTCM 3 final SSR format (RTCM 3
*                           Amendment 5) (ref [7]) (2.4.1_p6)
*           2012/05/14 1.6  separate rtcm2.c, rtcm3.c
*                           add options to select used codes for msm
*           2013/04/27 1.7  comply with rtcm 3.2 with amendment 1/2 (ref[15])
*           2013/12/06 1.8  support SBAS/BeiDou SSR messages (ref[16])
*           2018/01/29 1.9  support RTCM 3.3 (ref[17])
*                           crc24q() -> rtk_crc24q()
*           2018/10/10 1.10 fix bug on initializing rtcm struct
*                           add rtcm option -GALINAV, -GALFNAV
*           2018/11/05 1.11 add notes for api gen_rtcm3()
*-----------------------------------------------------------------------------*/
#include "rtklib.h"

/* function prototypes -------------------------------------------------------*/
extern int decode_rtcm2(rtcm_t *rtcm);
extern int decode_rtcm3(rtcm_t *rtcm);
extern int encode_rtcm3(rtcm_t *rtcm, int type, int sync);

/* constants -----------------------------------------------------------------*/

#define RTCM2PREAMB 0x66        /* rtcm ver.2 frame preamble */
#define RTCM3PREAMB 0xD3        /* rtcm ver.3 frame preamble */

/* initialize rtcm control -----------------------------------------------------
* initialize rtcm control struct and reallocate memory for observation and
* ephemeris buffer in rtcm control struct
* args   : rtcm_t *raw   IO     rtcm control struct
* return : status (1:ok,0:memory allocation error)
*-----------------------------------------------------------------------------*/
extern int init_rtcm(rtcm_t *rtcm)
{
    gtime_t time0={0};
    obsd_t data0={{0}};
    eph_t  eph0 ={0,-1,-1};
    geph_t geph0={0,-1};
    ssr_t ssr0={{{0}}};
    int i,j;

    //trace(3,"init_rtcm:\n");

    rtcm->staid=rtcm->stah=rtcm->seqno=rtcm->outtype=0;
    rtcm->time=rtcm->time_s=time0;
    rtcm->sta.name[0]=rtcm->sta.marker[0]='\0';
    rtcm->sta.antdes[0]=rtcm->sta.antsno[0]='\0';
    rtcm->sta.rectype[0]=rtcm->sta.recver[0]=rtcm->sta.recsno[0]='\0';
    rtcm->sta.antsetup=rtcm->sta.itrf=rtcm->sta.deltype=0;
    for (i=0;i<3;i++) {
        rtcm->sta.pos[i]=rtcm->sta.del[i]=0.0;
    }
    rtcm->sta.hgt=0.0;
    rtcm->dgps=nullptr;
    for (i=0;i<MAXSAT;i++) {
        rtcm->ssr[i]=ssr0;
    }
    rtcm->msg[0]=rtcm->msgtype[0]=rtcm->opt[0]='\0';
    for (i=0;i<6;i++) rtcm->msmtype[i][0]='\0';
    rtcm->obsflag=rtcm->ephsat=0;
    for (i=0;i<MAXSAT;i++) for (j=0;j<NFREQ+NEXOBS;j++) {
            rtcm->cp[i][j]=0.0;
            rtcm->lock[i][j]=rtcm->loss[i][j]=0;
            rtcm->lltime[i][j]=time0;
        }
    rtcm->nbyte=rtcm->nbit=rtcm->len=0;
    rtcm->word=0;
    for (i=0;i<100;i++) rtcm->nmsg2[i]=0;
    for (i=0;i<400;i++) rtcm->nmsg3[i]=0;

    rtcm->obs.data=nullptr;
    rtcm->nav.eph =nullptr;
    rtcm->nav.geph=nullptr;

    /* reallocate memory for observation and ephemris buffer */
    if (!(rtcm->obs.data=(obsd_t *)malloc(sizeof(obsd_t)*MAXOBS))||
        !(rtcm->nav.eph =(eph_t  *)malloc(sizeof(eph_t )*MAXSAT*2))||
        !(rtcm->nav.geph=(geph_t *)malloc(sizeof(geph_t)*MAXPRNGLO))) {
        free_rtcm(rtcm);
        return 0;
    }
    rtcm->obs.n=0;
    rtcm->nav.n=MAXSAT*2;
    rtcm->nav.ng=MAXPRNGLO;
    for (i=0;i<MAXOBS   ;i++) rtcm->obs.data[i]=data0;
    for (i=0;i<MAXSAT*2 ;i++) rtcm->nav.eph [i]=eph0;
    for (i=0;i<MAXPRNGLO;i++) rtcm->nav.geph[i]=geph0;
    return 1;
}
/* free rtcm control ----------------------------------------------------------
* free observation and ephemris buffer in rtcm control struct
* args   : rtcm_t *raw   IO     rtcm control struct
* return : none
*-----------------------------------------------------------------------------*/
extern void free_rtcm(rtcm_t *rtcm)
{
    //trace(3,"free_rtcm:\n");

    /* free memory for observation and ephemeris buffer */
    free(rtcm->obs.data); rtcm->obs.data=nullptr; rtcm->obs.n=0;
    free(rtcm->nav.eph ); rtcm->nav.eph =nullptr; rtcm->nav.n=0;
    free(rtcm->nav.geph); rtcm->nav.geph=nullptr; rtcm->nav.ng=0;
}
extern int gen_rtcm3(rtcm_t *rtcm, int type, int sync)
{
  uint32_t crc;
  int i=0;

  rtcm->nbit=rtcm->len=rtcm->nbyte=0;

  /* set preamble and reserved */
  setbitu(rtcm->buff,i, 8,RTCM3PREAMB); i+= 8;
  setbitu(rtcm->buff,i, 6,0          ); i+= 6;
  setbitu(rtcm->buff,i,10,0          ); i+=10;

  /* encode rtcm 3 message body */
  if (!encode_rtcm3(rtcm,type,sync)) return 0;

  /* padding to align 8 bit boundary */
  for (i=rtcm->nbit;i%8;i++) {
    setbitu(rtcm->buff,i,1,0);
  }
  /* message length (header+data) (bytes) */
  if ((rtcm->len=i/8)>=3+1024) {
    //trace(2,"generate rtcm 3 message length error len=%d\n",rtcm->len-3);
    rtcm->nbit=rtcm->len=0;
    return 0;
  }
  /* message length without header and parity */
  setbitu(rtcm->buff,14,10,rtcm->len-3);

  /* crc-24q */
  crc=rtk_crc24q(rtcm->buff,rtcm->len);
  setbitu(rtcm->buff,i,24,crc);

  /* length total (bytes) */
  rtcm->nbyte=rtcm->len+3;

  return 1;
}


