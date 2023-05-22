#ifndef WIN32
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "rtklib.h"

/* coordinate rotation matrix ------------------------------------------------*/
#define Rx(t,X) do { \
    (X)[0]=1.0; (X)[1]=(X)[2]=(X)[3]=(X)[6]=0.0; \
    (X)[4]=(X)[8]=cos(t); (X)[7]=sin(t); (X)[5]=-(X)[7]; \
} while (0)

#define Ry(t,X) do { \
    (X)[4]=1.0; (X)[1]=(X)[3]=(X)[5]=(X)[7]=0.0; \
    (X)[0]=(X)[8]=cos(t); (X)[2]=sin(t); (X)[6]=-(X)[2]; \
} while (0)

#define Rz(t,X) do { \
    (X)[8]=1.0; (X)[2]=(X)[5]=(X)[6]=(X)[7]=0.0; \
    (X)[0]=(X)[4]=cos(t); (X)[3]=sin(t); (X)[1]=-(X)[3]; \
} while (0)

/* astronomical arguments: f={l,l',F,D,OMG} (rad) ----------------------------*/
static void ast_args(double t, double *f)
{
  static const double fc[][5]={ /* coefficients for iau 1980 nutation */
      { 134.96340251, 1717915923.2178,  31.8792,  0.051635, -0.00024470},
      { 357.52910918,  129596581.0481,  -0.5532,  0.000136, -0.00001149},
      {  93.27209062, 1739527262.8478, -12.7512, -0.001037,  0.00000417},
      { 297.85019547, 1602961601.2090,  -6.3706,  0.006593, -0.00003169},
      { 125.04455501,   -6962890.2665,   7.4722,  0.007702, -0.00005939}
  };
  double tt[4];
  int i,j;

  for (tt[0]=t,i=1;i<4;i++) tt[i]=tt[i-1]*t;
  for (i=0;i<5;i++) {
    f[i]=fc[i][0]*3600.0;
    for (j=0;j<4;j++) f[i]+=fc[i][j+1]*tt[j];
    f[i]=fmod(f[i]*AS2R,2.0*PI);
  }
}
/* iau 1980 nutation ---------------------------------------------------------*/
static void nut_iau1980(double t, const double *f, double *dpsi, double *deps)
{
  static const double nut[106][10]={
      {   0,   0,   0,   0,   1, -6798.4, -171996, -174.2, 92025,   8.9},
      {   0,   0,   2,  -2,   2,   182.6,  -13187,   -1.6,  5736,  -3.1},
      {   0,   0,   2,   0,   2,    13.7,   -2274,   -0.2,   977,  -0.5},
      {   0,   0,   0,   0,   2, -3399.2,    2062,    0.2,  -895,   0.5},
      {   0,  -1,   0,   0,   0,  -365.3,   -1426,    3.4,    54,  -0.1},
      {   1,   0,   0,   0,   0,    27.6,     712,    0.1,    -7,   0.0},
      {   0,   1,   2,  -2,   2,   121.7,    -517,    1.2,   224,  -0.6},
      {   0,   0,   2,   0,   1,    13.6,    -386,   -0.4,   200,   0.0},
      {   1,   0,   2,   0,   2,     9.1,    -301,    0.0,   129,  -0.1},
      {   0,  -1,   2,  -2,   2,   365.2,     217,   -0.5,   -95,   0.3},
      {  -1,   0,   0,   2,   0,    31.8,     158,    0.0,    -1,   0.0},
      {   0,   0,   2,  -2,   1,   177.8,     129,    0.1,   -70,   0.0},
      {  -1,   0,   2,   0,   2,    27.1,     123,    0.0,   -53,   0.0},
      {   1,   0,   0,   0,   1,    27.7,      63,    0.1,   -33,   0.0},
      {   0,   0,   0,   2,   0,    14.8,      63,    0.0,    -2,   0.0},
      {  -1,   0,   2,   2,   2,     9.6,     -59,    0.0,    26,   0.0},
      {  -1,   0,   0,   0,   1,   -27.4,     -58,   -0.1,    32,   0.0},
      {   1,   0,   2,   0,   1,     9.1,     -51,    0.0,    27,   0.0},
      {  -2,   0,   0,   2,   0,  -205.9,     -48,    0.0,     1,   0.0},
      {  -2,   0,   2,   0,   1,  1305.5,      46,    0.0,   -24,   0.0},
      {   0,   0,   2,   2,   2,     7.1,     -38,    0.0,    16,   0.0},
      {   2,   0,   2,   0,   2,     6.9,     -31,    0.0,    13,   0.0},
      {   2,   0,   0,   0,   0,    13.8,      29,    0.0,    -1,   0.0},
      {   1,   0,   2,  -2,   2,    23.9,      29,    0.0,   -12,   0.0},
      {   0,   0,   2,   0,   0,    13.6,      26,    0.0,    -1,   0.0},
      {   0,   0,   2,  -2,   0,   173.3,     -22,    0.0,     0,   0.0},
      {  -1,   0,   2,   0,   1,    27.0,      21,    0.0,   -10,   0.0},
      {   0,   2,   0,   0,   0,   182.6,      17,   -0.1,     0,   0.0},
      {   0,   2,   2,  -2,   2,    91.3,     -16,    0.1,     7,   0.0},
      {  -1,   0,   0,   2,   1,    32.0,      16,    0.0,    -8,   0.0},
      {   0,   1,   0,   0,   1,   386.0,     -15,    0.0,     9,   0.0},
      {   1,   0,   0,  -2,   1,   -31.7,     -13,    0.0,     7,   0.0},
      {   0,  -1,   0,   0,   1,  -346.6,     -12,    0.0,     6,   0.0},
      {   2,   0,  -2,   0,   0, -1095.2,      11,    0.0,     0,   0.0},
      {  -1,   0,   2,   2,   1,     9.5,     -10,    0.0,     5,   0.0},
      {   1,   0,   2,   2,   2,     5.6,      -8,    0.0,     3,   0.0},
      {   0,  -1,   2,   0,   2,    14.2,      -7,    0.0,     3,   0.0},
      {   0,   0,   2,   2,   1,     7.1,      -7,    0.0,     3,   0.0},
      {   1,   1,   0,  -2,   0,   -34.8,      -7,    0.0,     0,   0.0},
      {   0,   1,   2,   0,   2,    13.2,       7,    0.0,    -3,   0.0},
      {  -2,   0,   0,   2,   1,  -199.8,      -6,    0.0,     3,   0.0},
      {   0,   0,   0,   2,   1,    14.8,      -6,    0.0,     3,   0.0},
      {   2,   0,   2,  -2,   2,    12.8,       6,    0.0,    -3,   0.0},
      {   1,   0,   0,   2,   0,     9.6,       6,    0.0,     0,   0.0},
      {   1,   0,   2,  -2,   1,    23.9,       6,    0.0,    -3,   0.0},
      {   0,   0,   0,  -2,   1,   -14.7,      -5,    0.0,     3,   0.0},
      {   0,  -1,   2,  -2,   1,   346.6,      -5,    0.0,     3,   0.0},
      {   2,   0,   2,   0,   1,     6.9,      -5,    0.0,     3,   0.0},
      {   1,  -1,   0,   0,   0,    29.8,       5,    0.0,     0,   0.0},
      {   1,   0,   0,  -1,   0,   411.8,      -4,    0.0,     0,   0.0},
      {   0,   0,   0,   1,   0,    29.5,      -4,    0.0,     0,   0.0},
      {   0,   1,   0,  -2,   0,   -15.4,      -4,    0.0,     0,   0.0},
      {   1,   0,  -2,   0,   0,   -26.9,       4,    0.0,     0,   0.0},
      {   2,   0,   0,  -2,   1,   212.3,       4,    0.0,    -2,   0.0},
      {   0,   1,   2,  -2,   1,   119.6,       4,    0.0,    -2,   0.0},
      {   1,   1,   0,   0,   0,    25.6,      -3,    0.0,     0,   0.0},
      {   1,  -1,   0,  -1,   0, -3232.9,      -3,    0.0,     0,   0.0},
      {  -1,  -1,   2,   2,   2,     9.8,      -3,    0.0,     1,   0.0},
      {   0,  -1,   2,   2,   2,     7.2,      -3,    0.0,     1,   0.0},
      {   1,  -1,   2,   0,   2,     9.4,      -3,    0.0,     1,   0.0},
      {   3,   0,   2,   0,   2,     5.5,      -3,    0.0,     1,   0.0},
      {  -2,   0,   2,   0,   2,  1615.7,      -3,    0.0,     1,   0.0},
      {   1,   0,   2,   0,   0,     9.1,       3,    0.0,     0,   0.0},
      {  -1,   0,   2,   4,   2,     5.8,      -2,    0.0,     1,   0.0},
      {   1,   0,   0,   0,   2,    27.8,      -2,    0.0,     1,   0.0},
      {  -1,   0,   2,  -2,   1,   -32.6,      -2,    0.0,     1,   0.0},
      {   0,  -2,   2,  -2,   1,  6786.3,      -2,    0.0,     1,   0.0},
      {  -2,   0,   0,   0,   1,   -13.7,      -2,    0.0,     1,   0.0},
      {   2,   0,   0,   0,   1,    13.8,       2,    0.0,    -1,   0.0},
      {   3,   0,   0,   0,   0,     9.2,       2,    0.0,     0,   0.0},
      {   1,   1,   2,   0,   2,     8.9,       2,    0.0,    -1,   0.0},
      {   0,   0,   2,   1,   2,     9.3,       2,    0.0,    -1,   0.0},
      {   1,   0,   0,   2,   1,     9.6,      -1,    0.0,     0,   0.0},
      {   1,   0,   2,   2,   1,     5.6,      -1,    0.0,     1,   0.0},
      {   1,   1,   0,  -2,   1,   -34.7,      -1,    0.0,     0,   0.0},
      {   0,   1,   0,   2,   0,    14.2,      -1,    0.0,     0,   0.0},
      {   0,   1,   2,  -2,   0,   117.5,      -1,    0.0,     0,   0.0},
      {   0,   1,  -2,   2,   0,  -329.8,      -1,    0.0,     0,   0.0},
      {   1,   0,  -2,   2,   0,    23.8,      -1,    0.0,     0,   0.0},
      {   1,   0,  -2,  -2,   0,    -9.5,      -1,    0.0,     0,   0.0},
      {   1,   0,   2,  -2,   0,    32.8,      -1,    0.0,     0,   0.0},
      {   1,   0,   0,  -4,   0,   -10.1,      -1,    0.0,     0,   0.0},
      {   2,   0,   0,  -4,   0,   -15.9,      -1,    0.0,     0,   0.0},
      {   0,   0,   2,   4,   2,     4.8,      -1,    0.0,     0,   0.0},
      {   0,   0,   2,  -1,   2,    25.4,      -1,    0.0,     0,   0.0},
      {  -2,   0,   2,   4,   2,     7.3,      -1,    0.0,     1,   0.0},
      {   2,   0,   2,   2,   2,     4.7,      -1,    0.0,     0,   0.0},
      {   0,  -1,   2,   0,   1,    14.2,      -1,    0.0,     0,   0.0},
      {   0,   0,  -2,   0,   1,   -13.6,      -1,    0.0,     0,   0.0},
      {   0,   0,   4,  -2,   2,    12.7,       1,    0.0,     0,   0.0},
      {   0,   1,   0,   0,   2,   409.2,       1,    0.0,     0,   0.0},
      {   1,   1,   2,  -2,   2,    22.5,       1,    0.0,    -1,   0.0},
      {   3,   0,   2,  -2,   2,     8.7,       1,    0.0,     0,   0.0},
      {  -2,   0,   2,   2,   2,    14.6,       1,    0.0,    -1,   0.0},
      {  -1,   0,   0,   0,   2,   -27.3,       1,    0.0,    -1,   0.0},
      {   0,   0,  -2,   2,   1,  -169.0,       1,    0.0,     0,   0.0},
      {   0,   1,   2,   0,   1,    13.1,       1,    0.0,     0,   0.0},
      {  -1,   0,   4,   0,   2,     9.1,       1,    0.0,     0,   0.0},
      {   2,   1,   0,  -2,   0,   131.7,       1,    0.0,     0,   0.0},
      {   2,   0,   0,   2,   0,     7.1,       1,    0.0,     0,   0.0},
      {   2,   0,   2,  -2,   1,    12.8,       1,    0.0,    -1,   0.0},
      {   2,   0,  -2,   0,   1,  -943.2,       1,    0.0,     0,   0.0},
      {   1,  -1,   0,  -2,   0,   -29.3,       1,    0.0,     0,   0.0},
      {  -1,   0,   0,   1,   1,  -388.3,       1,    0.0,     0,   0.0},
      {  -1,  -1,   0,   2,   1,    35.0,       1,    0.0,     0,   0.0},
      {   0,   1,   0,   1,   0,    27.3,       1,    0.0,     0,   0.0}
  };
  double ang;
  int i,j;

  *dpsi=*deps=0.0;

  for (i=0;i<106;i++) {
    ang=0.0;
    for (j=0;j<5;j++) ang+=nut[i][j]*f[j];
    *dpsi+=(nut[i][6]+nut[i][7]*t)*sin(ang);
    *deps+=(nut[i][8]+nut[i][9]*t)*cos(ang);
  }
  *dpsi*=1E-4*AS2R; /* 0.1 mas -> rad */
  *deps*=1E-4*AS2R;
}
/* eci to ecef transformation matrix -------------------------------------------
* compute eci to ecef transformation matrix
* args   : gtime_t tutc     I   time in utc
*          double *erpv     I   erp values {xp,yp,ut1_utc,lod} (rad,rad,s,s/d)
*          double *U        O   eci to ecef transformation matrix (3 x 3)
*          double *gmst     IO  greenwich mean sidereal time (rad)
*                               (NULL: no output)
* return : none
* note   : see ref [3] chap 5
*          not thread-safe
*-----------------------------------------------------------------------------*/
extern void eci2ecef(gtime_t tutc, const double *erpv, double *U, double *gmst)
{
  const double ep2000[]={2000,1,1,12,0,0};
  static gtime_t tutc_;
  static double U_[9],gmst_;
  gtime_t tgps;
  double eps,ze,th,z,t,t2,t3,dpsi,deps,gast,f[5];
  double R1[9],R2[9],R3[9],R[9],W[9],N[9],P[9],NP[9];
  int i;
  if (fabs(timediff(tutc,tutc_))<0.01) { /* read cache */
    for (i=0;i<9;i++) U[i]=U_[i];
    if (gmst) *gmst=gmst_;
    return;
  }
  tutc_=tutc;

  /* terrestrial time */
  tgps=utc2gpst(tutc_);
  t=(timediff(tgps,epoch2time(ep2000))+19.0+32.184)/86400.0/36525.0;
  t2=t*t; t3=t2*t;

  /* astronomical arguments */
  ast_args(t,f);

  /* iau 1976 precession */
  ze=(2306.2181*t+0.30188*t2+0.017998*t3)*AS2R;
  th=(2004.3109*t-0.42665*t2-0.041833*t3)*AS2R;
  z =(2306.2181*t+1.09468*t2+0.018203*t3)*AS2R;
  eps=(84381.448-46.8150*t-0.00059*t2+0.001813*t3)*AS2R;
  Rz(-z,R1); Ry(th,R2); Rz(-ze,R3);
  matmul("NN",3,3,3,1.0,R1,R2,0.0,R);
  matmul("NN",3,3,3,1.0,R, R3,0.0,P); /* P=Rz(-z)*Ry(th)*Rz(-ze) */

  /* iau 1980 nutation */
  nut_iau1980(t,f,&dpsi,&deps);
  Rx(-eps-deps,R1); Rz(-dpsi,R2); Rx(eps,R3);
  matmul("NN",3,3,3,1.0,R1,R2,0.0,R);
  matmul("NN",3,3,3,1.0,R ,R3,0.0,N); /* N=Rx(-eps)*Rz(-dspi)*Rx(eps) */

  /* greenwich aparent sidereal time (rad) */
  gmst_=utc2gmst(tutc_,erpv[2]);
  gast=gmst_+dpsi*cos(eps);
  gast+=(0.00264*sin(f[4])+0.000063*sin(2.0*f[4]))*AS2R;

  /* eci to ecef transformation matrix */
  Ry(-erpv[0],R1); Rx(-erpv[1],R2); Rz(gast,R3);
  matmul("NN",3,3,3,1.0,R1,R2,0.0,W );
  matmul("NN",3,3,3,1.0,W ,R3,0.0,R ); /* W=Ry(-xp)*Rx(-yp) */
  matmul("NN",3,3,3,1.0,N ,P ,0.0,NP);
  matmul("NN",3,3,3,1.0,R ,NP,0.0,U_); /* U=W*Rz(gast)*N*P */

  for (i=0;i<9;i++) U[i]=U_[i];
  if (gmst) *gmst=gmst_;

}

/* sun and moon position in eci (ref [4] 5.1.1, 5.2.1) -----------------------*/
static void sunmoonpos_eci(gtime_t tut, double *rsun, double *rmoon)
{
  const double ep2000[]={2000,1,1,12,0,0};
  double t,f[5],eps,Ms,ls,rs,lm,pm,rm,sine,cose,sinp,cosp,sinl,cosl;

  t=timediff(tut,epoch2time(ep2000))/86400.0/36525.0;

  /* astronomical arguments */
  ast_args(t,f);

  /* obliquity of the ecliptic */
  eps=23.439291-0.0130042*t;
  sine=sin(eps*D2R); cose=cos(eps*D2R);

  /* sun position in eci */
  if (rsun) {
    Ms=357.5277233+35999.05034*t;
    ls=280.460+36000.770*t+1.914666471*sin(Ms*D2R)+0.019994643*sin(2.0*Ms*D2R);
    rs=AU*(1.000140612-0.016708617*cos(Ms*D2R)-0.000139589*cos(2.0*Ms*D2R));
    sinl=sin(ls*D2R); cosl=cos(ls*D2R);
    rsun[0]=rs*cosl;
    rsun[1]=rs*cose*sinl;
    rsun[2]=rs*sine*sinl;
  }
  /* moon position in eci */
  if (rmoon) {
    lm=218.32+481267.883*t+6.29*sin(f[0])-1.27*sin(f[0]-2.0*f[3])+
        0.66*sin(2.0*f[3])+0.21*sin(2.0*f[0])-0.19*sin(f[1])-0.11*sin(2.0*f[2]);
    pm=5.13*sin(f[2])+0.28*sin(f[0]+f[2])-0.28*sin(f[2]-f[0])-
        0.17*sin(f[2]-2.0*f[3]);
    rm=RE_WGS84/sin((0.9508+0.0518*cos(f[0])+0.0095*cos(f[0]-2.0*f[3])+
        0.0078*cos(2.0*f[3])+0.0028*cos(2.0*f[0]))*D2R);
    sinl=sin(lm*D2R); cosl=cos(lm*D2R);
    sinp=sin(pm*D2R); cosp=cos(pm*D2R);
    rmoon[0]=rm*cosp*cosl;
    rmoon[1]=rm*(cose*cosp*sinl-sine*sinp);
    rmoon[2]=rm*(sine*cosp*sinl+cose*sinp);
  }
}

/* sun and moon position -------------------------------------------------------
* get sun and moon position in ecef
* args   : gtime_t tut      I   time in ut1
*          double *erpv     I   erp value {xp,yp,ut1_utc,lod} (rad,rad,s,s/d)
*          double *rsun     IO  sun position in ecef  (m) (NULL: not output)
*          double *rmoon    IO  moon position in ecef (m) (NULL: not output)
*          double *gmst     O   gmst (rad)
* return : none
*-----------------------------------------------------------------------------*/
extern void sunmoonpos(gtime_t tutc, const double *erpv, double *rsun,
                       double *rmoon, double *gmst)
{
  gtime_t tut;
  double rs[3],rm[3],U[9],gmst_;

  tut=timeadd(tutc,erpv[2]); /* utc -> ut1 */

  /* sun and moon position in eci */
  sunmoonpos_eci(tut,rsun?rs:NULL,rmoon?rm:NULL);

  /* eci to ecef transformation matrix */
  eci2ecef(tutc,erpv,U,&gmst_);

  /* sun and moon postion in ecef */
  if (rsun ) matmul("NN",3,1,3,1.0,U,rs,0.0,rsun );
  if (rmoon) matmul("NN",3,1,3,1.0,U,rm,0.0,rmoon);
  if (gmst ) *gmst=gmst_;
}
/* nominal yaw-angle ---------------------------------------------------------*/
static double yaw_nominal(double beta, double mu)
{
  if (fabs(beta)<1E-12&&fabs(mu)<1E-12) return PI;
  return atan2(-tan(beta),sin(mu))+PI;
}
/* yaw-angle of satellite ----------------------------------------------------*/
extern int yaw_angle(int opt, double beta, double mu,
                     double *yaw)
{
  *yaw=yaw_nominal(beta,mu);
  return 1;
}
/* satellite attitude model --------------------------------------------------*/
static int sat_yaw(gtime_t time, int opt,
                   const double *rs, double *exs, double *eys)
{
  double rsun[3],ri[6],es[3],esun[3],n[3],p[3],en[3],ep[3],ex[3],E,beta,mu;
  double yaw,cosy,siny,erpv[5]={0};
  int i;

  sunmoonpos(gpst2utc(time),erpv,rsun,NULL,NULL);

  /* beta and orbit angle */
  matcpy(ri,rs,6,1);
  ri[3]-=OMGE*ri[1];
  ri[4]+=OMGE*ri[0];
  cross3(ri,ri+3,n);
  cross3(rsun,n,p);
  if (!normv3(rs,es)||!normv3(rsun,esun)||!normv3(n,en)||
      !normv3(p,ep)) return 0;
  beta=PI/2.0-acos(dot(esun,en,3));
  E=acos(dot(es,ep,3));
  mu=PI/2.0+(dot(es,esun,3)<=0?-E:E);
  if      (mu<-PI/2.0) mu+=2.0*PI;
  else if (mu>=PI/2.0) mu-=2.0*PI;

  /* yaw-angle of satellite */
  if (!yaw_angle(opt,beta,mu,&yaw)) return 0;

  /* satellite fixed x,y-vector */
  cross3(en,es,ex);
  cosy=cos(yaw);
  siny=sin(yaw);
  for (i=0;i<3;i++) {
    exs[i]=-siny*en[i]+cosy*ex[i];
    eys[i]=-cosy*en[i]-siny*ex[i];
  }
  return 1;
}
/* phase windup model --------------------------------------------------------
 * time: current receiver time in GPST
 *  rs is the sat positoin in ECEF at transmit time
 *  rr is the user position in ECEF
 *  opt = 1
*/
extern int model_phw(gtime_t time, int opt,
                     const double *rs, const double *rr, double &phw)
{
  double exs[3],eys[3],ek[3],exr[3],eyr[3],eks[3],ekr[3],E[9];
  double dr[3],ds[3],drs[3],r[3],pos[3],cosp,ph;
  int i;

  if (opt<=0) return 1; /* no phase windup */

  /* satellite yaw attitude model */
  if (!sat_yaw(time,opt,rs,exs,eys)) return 0;

  /* unit vector satellite to receiver */
  for (i=0;i<3;i++) r[i]=rr[i]-rs[i];
  if (!normv3(r,ek)) return 0;

  /* unit vectors of receiver antenna */
  ecef2pos(rr,pos);
  xyz2enu(pos,E);
  exr[0]= E[1]; exr[1]= E[4]; exr[2]= E[7]; /* x = north */
  eyr[0]=-E[0]; eyr[1]=-E[3]; eyr[2]=-E[6]; /* y = west  */

  /* phase windup effect */
  cross3(ek,eys,eks);
  cross3(ek,eyr,ekr);
  for (i=0;i<3;i++) {
    ds[i]=exs[i]-ek[i]*dot(ek,exs,3)-eks[i];
    dr[i]=exr[i]-ek[i]*dot(ek,exr,3)+ekr[i];
  }
  cosp=dot(ds,dr,3)/norm(ds,3)/norm(dr,3);
  if      (cosp<-1.0) cosp=-1.0;
  else if (cosp> 1.0) cosp= 1.0;
  ph=acos(cosp)/2.0/PI;
  cross3(ds,dr,drs);
  if (dot(ek,drs,3)<0.0) ph=-ph;

  phw=ph+floor(phw-ph+0.5); /* in cycle */
  return 1;
}