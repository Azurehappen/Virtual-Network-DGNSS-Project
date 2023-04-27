
#ifndef WADGNSS_SERVER_SSR_VTEC_H
#define WADGNSS_SERVER_SSR_VTEC_H
#include "bkg_data_requestor.h"
#include "rtklib.h"
class SSR_VTEC {
public:
  SSR_VTEC();
  ~SSR_VTEC();
  double stec(const VTecCorrection& tec, double gpst_sec, const std::vector<double>& r_ecef,
              const std::vector<double>& xyzSat, double sys_F1);

private:
  static void xyz2neu(double* Ell, const double* xyz, double* neu);
  static void satazel(const double *pos,const double *sat, double *azel);
  double vtecSingleLayerContribution(const VTecCorrection& tec) const;
  void piercePoint(double layerHeight, double epoch, const double* geocSta,
                   double r, const double *azel);
  double _psiPP;
  double _phiPP;
  double _lambdaPP;
  double _lonS;
};

#endif // WADGNSS_SERVER_SSR_VTEC_H
