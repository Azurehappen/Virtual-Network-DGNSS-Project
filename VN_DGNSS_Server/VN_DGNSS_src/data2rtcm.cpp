#include "data2rtcm.h"

#define NINCOBS 262144 /* inclimental number of obs data */

/* test rtcm nav data --------------------------------------------------------*/
static int is_nav(int type) {
  return type == 1019 || type == 1044 || type == 1045 || type == 1046;
}
/* test rtcm gnav data -------------------------------------------------------*/
static int is_gnav(int type) { return type == 1020; }
/* test rtcm ant info --------------------------------------------------------*/
static int is_ant(int type) {
  return type == 1005 || type == 1006 || type == 1007 || type == 1008 ||
         type == 1033;
}
/* initialize station parameter ----------------------------------------------*/
static void init_sta(sta_t *sta) {
  int i;
  *sta->name = '\0';
  *sta->marker = '\0';
  *sta->antdes = '\0';
  *sta->antsno = '\0';
  *sta->rectype = '\0';
  *sta->recver = '\0';
  *sta->recsno = '\0';
  sta->antsetup = sta->itrf = sta->deltype = 0;
  for (i = 0; i < 3; i++) sta->pos[i] = 0.0;
  for (i = 0; i < 3; i++) sta->del[i] = 0.0;
  sta->hgt = 0.0;
}
/* generate rtcm obs data messages -------------------------------------------*/
static void gen_rtcm_obs(rtcm_t *rtcm, const int *type,
                         int n, SockRTCM *client_info) {
  int i, j = 0;
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_info->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
  for (i = 0; i < n; i++) {
    if (is_nav(type[i]) || is_gnav(type[i]) || is_ant(type[i])) continue;
    j = i; /* index of last message */
  }
  for (i = 0; i < n; i++) {
    if (is_nav(type[i]) || is_gnav(type[i]) || is_ant(type[i])) continue;

    if (!gen_rtcm3(rtcm, type[i], i != j)) continue;
    // if (fwrite(rtcm->buff,rtcm->nbyte,1,fp)<1) break;
    int ret = send(client_info->fd, rtcm->buff, rtcm->nbyte, MSG_NOSIGNAL);
    if (ret == -1){
      *client_info->log << get_time() <<"Client IP: "<< client_ip
                        << ", Port: "<< ntohs(client_info->addr.sin_port)
                        << " send error: " << strerror(errno)
                        << std::endl;
      client_info->send_check = false;
    }
    else if(ret == 0){
      *client_info->log << get_time() <<"client IP: "<< client_ip
                        << ", Port: "<< ntohs(client_info->addr.sin_port)
                        << " disconnected when sending data " << strerror(errno)
                        << std::endl;
    }
  }
}
/* generate rtcm antenna info messages ---------------------------------------*/
static void gen_rtcm_ant(rtcm_t *rtcm, const int *type,
                         int n, SockRTCM *client_info) {
  int i;
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_info->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
  for (i = 0; i < n; i++) {
    if (!is_ant(type[i])) continue;

    if (!gen_rtcm3(rtcm, type[i], 0)) continue;
    // if (fwrite(rtcm->buff,rtcm->nbyte,1,fp)<1) break;
    int ret = send(client_info->fd, rtcm->buff, rtcm->nbyte, MSG_NOSIGNAL);
    if (ret == -1){
      *client_info->log << get_time() <<"client IP: "<< client_ip
                        << ", Port: "<< ntohs(client_info->addr.sin_port)
                        << " send error: " << strerror(errno)
                        << std::endl;
      client_info->send_check = false;
    }
    else if(ret == 0){
      *client_info->log << get_time() <<"client IP: "<< client_ip
                        << ", Port: "<< ntohs(client_info->addr.sin_port)
                        << " disconnected when sending data " << strerror(errno)
                        << std::endl;
    }
  }
}
/* restore slips -------------------------------------------------------------*/
static void restslips(unsigned char slips[][NFREQ], obsd_t *data) {
  int i;
  for (i = 0; i < NFREQ; i++) {
    if (slips[data->sat - 1][i] & 1) data->LLI[i] |= LLI_SLIP;
    slips[data->sat - 1][i] = 0;
  }
}
/* add obs data --------------------------------------------------------------*/
static int addobsdata(obs_t *obs, const obsd_t *data) {
  obsd_t *obs_data;

  if (obs->nmax <= obs->n) {
    if (obs->nmax <= 0)
      obs->nmax = NINCOBS;
    else
      obs->nmax *= 2;
    if (!(obs_data =
              (obsd_t *)realloc(obs->data, sizeof(obsd_t) * obs->nmax))) {
      // trace(1,"addobsdata: memalloc error
      // n=%dx%d\n",sizeof(obsd_t),obs->nmax);
      free(obs->data);
      obs->data = nullptr;
      obs->n = obs->nmax = 0;
      return -1;
    }
    obs->data = obs_data;
  }
  obs->data[obs->n++] = *data;
  return 1;
}
/* convert to rtcm messages --------------------------------------------------*/
static int conv_rtcm(const int *type, int n, SockRTCM *client_info,
                     const obs_t *obs, const nav_t *nav, const sta_t *sta,
                     int staid)  // vector<vector<gtime_t>> &lltime_rcd)
{
  // FILE *fp=stdout;
  gtime_t time0 = {0};
  rtcm_t rtcm = {0};
  eph_t eph0 = {0};
  geph_t geph0 = {0};
  /*
  for (int u = 0;u<MAXSAT;u++){
      for (int ui = 0;ui<NFREQ+NEXOBS;ui++){
          rtcm.lltime[u][ui] = lltime_rcd[u][ui];
      }
  }*/
  int i, j, prn, index[2] = {0};

  if (!(rtcm.nav.eph = (eph_t *)malloc(sizeof(eph_t) * MAXSAT)) ||
      !(rtcm.nav.geph = (geph_t *)malloc(sizeof(geph_t) * MAXPRNGLO)))
    return 0;

  rtcm.staid = staid;
  rtcm.sta = *sta;

  for (i = 0; i < MAXSAT; i++) rtcm.nav.eph[i] = eph0;
  for (i = 0; i < MAXPRNGLO; i++) rtcm.nav.geph[i] = geph0;
  /* update glonass freq channel number */
  for (i = 0; i < nav->ng; i++) {
    if (satsys(nav->geph[i].sat, &prn) != SYS_GLO) continue;
    rtcm.nav.geph[prn - 1] = nav->geph[i];
  }
  /* gerate rtcm antenna info messages */
  gen_rtcm_ant(&rtcm, type, n, client_info);

  for (i = 0; i < obs->n; i = j) {
    /* extract epoch obs data */
    for (j = i + 1; j < obs->n; j++) {
      if (timediff(obs->data[j].time, obs->data[i].time) > DTTOL) {
        std::cerr << "Dt out" << std::endl;
        break;
      }
    }
    rtcm.time = obs->data[i].time;
    rtcm.seqno++;
    rtcm.obs.data = obs->data + i;
    rtcm.obs.n = j - i;
    /* generate rtcm obs data messages */
    gen_rtcm_obs(&rtcm, type, n, client_info);
    // fprintf(stderr,"NOBS=%2d\r",rtcm.obs.n);
  }

  /* print statistics  */
  // fprintf(stderr,"\n  MT  # OF MSGS\n");
  // memcpy(lltime_rcd.data(),rtcm.lltime,sizeof(lltime_rcd[0][0])*MAXSAT*(NFREQ+NEXOBS));
  /*for (int u = 0;u<MAXSAT;u++){
      for (int ui = 0;ui<NFREQ+NEXOBS;ui++){
          lltime_rcd[u][ui] = rtcm.lltime[u][ui];
      }
  }*/
  //    for (i=1;i<299;i++) {
  //        if (!rtcm.nmsg3[i]) continue;
  //        fprintf(stderr,"%04d %10d\n",1000+i,rtcm.nmsg3[i]);
  //    }
  //    fprintf(stderr,"\n");
  free(rtcm.nav.eph);
  free(rtcm.nav.geph);
  return 1;
}
/* main ----------------------------------------------------------------------*/
int data2rtcm(int n, const int *type, int m, SockRTCM *client_info,
              std::vector<double> sta_pos, std::vector<obsd_t> data_obs) {
  int staid = 0; /*Station ID*/
  nav_t nav = {0};
  int stat = 0;
  sta_t sta = {{0}};
  int i, ret = 0;
  // insert data example:
  /* Initialize RTKLIB struct */
  obs_t obs = {0};
  obsd_t *data;
  data = (obsd_t *)malloc(sizeof(obsd_t) *
          MAXOBS); /*See in RTKLIB func: readrnxobs*/
  for (i = 0; i < MAXOBS; i++) {
    data[i].time = data_obs[i].time;
    data[i].sat = data_obs[i].sat;
    int prn,sv;
    sv = satsys(data[i].sat,&prn);
//    *client_info->rtcm_log << "Sat: " << (int)data[i].sat
//                          << " SV: " << sv << " PRN: " << prn << std::endl;
    data[i].rcv = data_obs[i].rcv;
    for (int k = 0; k < NFREQ + NEXOBS; k++) {
      data[i].P[k] = data_obs[i].P[k];
      data[i].L[k] = data_obs[i].L[k];
      data[i].D[k] = data_obs[i].D[k];
      data[i].SNR[k] = data_obs[i].SNR[k];
      data[i].LLI[k] = data_obs[i].LLI[k];
      data[i].code[k] = data_obs[i].code[k];
      data[i].lockt[k] = data_obs[i].lockt[k];
//      if (i < n) {
//        *client_info->rtcm_log << "Code: " << data[i].P[k]
//                              << " Phase: " << data[i].L[k]
//                              << " SNR: " << (int)data[i].SNR[k]
//                              << " LLI: " << (int)data[i].LLI[k]
//                              << " channel: " << (int)data[i].code[k]
//                              << " lockt: " << data[i].lockt[k] << std::endl;
//      }
    }
  }
  /*Save data tp obs struct*/
  for (i = 0; i < n; i++) {
    /* save obs data */
    if ((stat = addobsdata(&obs, data + i)) < 0) break;
  }
  free(data);
  /* Generate "sta" */
  init_sta(&sta);
  for (int j = 0; j < 3; j++) sta.pos[j] = sta_pos[j];
  sta.del[0] = 0;
  sta.del[1] = 0;
  sta.del[2] = 0;
  sta.name[0] = 'U';
  sta.name[1] = 'C';
  sta.name[2] = 'R';

  /*Convert to RTCM message*/
  // int type[16];
  // int m = 2, staid = 0; /*Number of message type, Station ID*/
  // type[0] = 1001; type[1]=1005; /* Define massage type */
  sortobs(&obs);

  /* convert to rtcm messages */
  if (!conv_rtcm(type, m, client_info, &obs, &nav, &sta, staid)) ret = -1;

  free(obs.data);
  freenav(&nav, 0xFF);

  return ret;
}

// int main(){
//    double date_time_gps[6]={2020,6, 18, 5, 43, 00};
//    vector<unsigned char> prn_recd(8,156);
//    int n=8; // Number of satellites available
//    for(int i=1;i<9;i++){
//        prn_recd.push_back(i);
//
//    }
//    char const *outfile="";
//    int type[16];
//    int m = 2, staid = 0; /*Number of message type, Station ID*/
//    type[0] = 1001; type[1]=1005; /* Define massage type */
//    int ret = data2rtcm(date_time_gps,n,type,m,outfile,staid);
//}