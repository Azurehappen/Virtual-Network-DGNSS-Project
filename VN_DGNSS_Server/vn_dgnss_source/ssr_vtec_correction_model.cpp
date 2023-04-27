#include "ssr_vtec_correction_model.h"
#define min(x,y)    ((x)<(y)?(x):(y))

double factorial(int n) {
  if (n == 0) {
    return 1;
  }
  else {
    return (n * factorial(n - 1));
  }
}
double associatedLegendreFunction(int n, int m, double t) {
  double sum = 0.0;
  int    r   = (int) floor((n - m) / 2);
  for (int k = 0; k <= r; k++) {
    sum += (pow(-1.0, (double)k) * factorial(2*n - 2*k)
            / (factorial(k) * factorial(n-k) * factorial(n-m-2*k))
            * pow(t, (double)n-m-2*k));
  }
  double fac = pow(2.0,(double) -n) * pow((1 - t*t), (double)m/2);
  return sum *= fac;
}
// Constructor
SsrVtecCorrectionModel::SsrVtecCorrectionModel() {
  _psiPP = _phiPP = _lambdaPP = _lonS = 0.0;
}

void SsrVtecCorrectionModel::xyz2neu(double* Ell, const double* xyz, double* neu) {

  double sinPhi = sin(Ell[0]);
  double cosPhi = cos(Ell[0]);
  double sinLam = sin(Ell[1]);
  double cosLam = cos(Ell[1]);

  neu[0] = - sinPhi*cosLam * xyz[0]
           - sinPhi*sinLam * xyz[1]
           + cosPhi        * xyz[2];

  neu[1] = - sinLam * xyz[0]
           + cosLam * xyz[1];

  neu[2] = + cosPhi*cosLam * xyz[0]
           + cosPhi*sinLam * xyz[1]
           + sinPhi        * xyz[2];
}
void SsrVtecCorrectionModel::satazel(const double *r_ecef,const double *sat, double *azel) {
  double rhoV[3];
  for (int i=0;i<3;i++) {
    rhoV[i] = sat[i] - r_ecef[i];
  }
  double rho = sqrt(rhoV[0]*rhoV[0]+rhoV[1]*rhoV[1]+rhoV[2]*rhoV[2]);
  double neu[3];
  double geocSta[3];
  ecef2pos(r_ecef,geocSta);
  xyz2neu(geocSta, rhoV, neu);
  double sphEle = acos( sqrt(neu[0]*neu[0] + neu[1]*neu[1]) / rho );
  if (neu[2] < 0) {
    sphEle *= -1.0;
  }
  double sphAzi = atan2(neu[1], neu[0]);
  azel[0] = sphAzi;
  azel[1] = sphEle;
}

void SsrVtecCorrectionModel::piercePoint(double layerHeight, double epoch, const double* geocSta,
            double r, const double *azel) {

  double sphEle = azel[1];
  double sphAzi = azel[0];
  double q = r / (6370000.0 + layerHeight);

  _psiPP = PI/2 - sphEle - asin(q * cos(sphEle));

  _phiPP = asin(sin(geocSta[0]) * cos(_psiPP) + cos(geocSta[0]) * sin(_psiPP) * cos(sphAzi));

  if (( (geocSta[0]*180.0/M_PI > 0) && (  tan(_psiPP) * cos(sphAzi)  > tan(M_PI/2 - geocSta[0])) )  ||
      ( (geocSta[0]*180.0/M_PI < 0) && (-(tan(_psiPP) * cos(sphAzi)) > tan(M_PI/2 + geocSta[0])) ))  {
    _lambdaPP = geocSta[1] + M_PI - asin((sin(_psiPP) * sin(sphAzi) / cos(_phiPP)));
  } else {
    _lambdaPP = geocSta[1]        + asin((sin(_psiPP) * sin(sphAzi) / cos(_phiPP)));
  }

  _lonS = fmod((_lambdaPP + (epoch - 50400) * M_PI / 43200), 2*M_PI);
}

double SsrVtecCorrectionModel::vtecSingleLayerContribution(const VTecCorrection& tec) const {

  double vtec = 0.0;
  int N = tec.nDeg;
  int M = tec.nOrd;
  double fac;

  for (int n = 0; n <= N; n++) {
    for (int m = 0; m <= min(n, M); m++) {
      double pnm = associatedLegendreFunction(n, m, sin(_phiPP));
      auto a = double(factorial(n - m));
      auto b = double(factorial(n + m));
      if (m == 0) {
        fac = sqrt(2.0 * n + 1);
      }
      else {
        fac = sqrt(2.0 * (2.0 * n + 1) * a / b);
      }
      pnm *= fac;
      double Cnm_mlambda = tec.cos_coeffs[n][m] * cos(m * _lonS);
      double Snm_mlambda = tec.sin_coeffs[n][m] * sin(m * _lonS);
      vtec += (Snm_mlambda + Cnm_mlambda) * pnm;
    }
  }

  if (vtec < 0.0) {
    return 0.0;
  }

  return vtec;
}

double SsrVtecCorrectionModel::stec(const VTecCorrection& tec, double gpst_sec, const std::vector<double>& r_ecef,
                      const std::vector<double>& xyzSat, double sys_F1) {

  // Latitude, longitude, height are defined with respect to a spherical earth model
  // -------------------------------------------------------------------------------
  double geocSta[3];
  ecef2pos(r_ecef.data(), geocSta);

  // elevation and azimuth with respect to a spherical earth model
  // -------------------------------------------------------------
  double azel[2];
  satazel(r_ecef.data(), xyzSat.data(), azel);

  double epoch = fmod(gpst_sec, 86400.0);
  double user_r = sqrt(r_ecef[0]*r_ecef[0]+
                       r_ecef[1]*r_ecef[1]+r_ecef[2]*r_ecef[2]);
  double stec = 0.0;
  piercePoint(tec.height_m, epoch, geocSta, user_r,azel);
  double vtec = vtecSingleLayerContribution(tec);
  stec += vtec / sin(azel[1] + _psiPP);
  return stec*40.3e16/sys_F1/sys_F1;
}
SsrVtecCorrectionModel::~SsrVtecCorrectionModel() = default;
