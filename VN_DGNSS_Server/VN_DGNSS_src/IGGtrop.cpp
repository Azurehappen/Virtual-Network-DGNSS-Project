#include "IGGtrop.h"

int IGGtrop::fix(double val) {
  if (val>=0) {
    return floor(val);
  } else {
    return ceil(val);
  }
}

IGGexpModel getIGGtropData(const std::string& file) {
  int nHPara = 6;
  int nPara = 5;
  double dlon = 2.5, dlat = 2.5;
  int nLon = 144; int nLat = 73;
  std::vector<float> data;
  std::vector<float> dataread(nHPara*nPara*nLat*nLon,0);
  std::ifstream inFile(file,std::ios::in|std::ios::binary);
  while(inFile.read((char *)dataread.data(), dataread.size())) {
    data.insert(data.end(),dataread.begin(),dataread.begin()+78840);
  }
  inFile.close();
  IGGexpModel TropModel;
  TropModel.data.resize(nPara);
  for (int n1 = 0; n1 < nPara;n1++) {
    TropModel.data[n1].resize(nHPara);
    for (int n2 = 0; n2 < nHPara; n2++) {
      TropModel.data[n1][n2].resize(nLat);
    }
  }
  // Reshape
  int cout=0;
  for (int n1 = 0; n1 < nPara;n1++) {
    for (int n2 = 0; n2 < nHPara; n2++) {
      for (int n3 = 0; n3 < nLat; n3++) {
        TropModel.data[n1][n2][n3].insert(TropModel.data[n1][n2][n3].end(),
                                          data.begin()+cout,
                                          data.begin()+144+cout);
        cout += 144;
      }
    }
  }
  return TropModel;
}
double IGGtrop::IGGtropdelay(
    double uLon, double uLat, double uH, int doy, double elev,
    const std::vector<std::vector<std::vector<std::vector<float>>>> &data) {
  int nHPara = 6;
  int nPara = 5;
  double dlon = 2.5, dlat = 2.5;
  int nLon = 144; int nLat = 73;
  double ix = (uLon - 0.0)/2.5+1;
  double iy = -(uLat - 90)/2.5+1;

  std::vector<double> Ygrid(4,0);
  double bit[4] = {0,1,0,1};
  for (int i = 0; i<4; i++) {
    int a = (fix(ix+bit[i])-1)%nLon+1;
    int b = fix(iy) + fix(i/2);
    if (b>nLat) {
      b = nLat;
    }
    std::vector<std::vector<float>> a_coe;
    a_coe.resize(nPara);
    for (int n1 = 0; n1 < nPara;n1++) {
      a_coe[n1].resize(nHPara);
    }
    for (int n1 = 0; n1 < nPara;n1++) {
      for (int n2 = 0; n2 < nHPara; n2++) {
        a_coe[n1][n2] = data[n1][n2][fix(b) - 1][fix(a) - 1];
      }
    }
    double ab1 = 0;
    for (int k = 1; k < nHPara-1; k++) {
      ab1 += a_coe[0][k]*pow(uH,k);
    }
    double ave = a_coe[0][0]*exp(ab1)+a_coe[0][nHPara-1];

    double ab2 = 0;
    std::vector<double> A(nPara-1,0);
    for (int n = 0; n < nPara-1;n++) {
      for (int k=0;k<nHPara;k++){
        A[n] += a_coe[n+1][k]*pow(uH,k);
      }
    }
    double t = 2*PI*doy/365.25;
    Ygrid[i] = ave+A[0]*cos(t)+A[1]*sin(t)+A[2]*cos(2*t)+A[3]*sin(2*t);
  }
  double p = ix - fix(ix);
  double q = iy - fix(iy);
  double ZTD = (1-p)*(1-q)*Ygrid[0]+(1-q)*p*Ygrid[1]+(1-p)*q*Ygrid[2]+p*q*Ygrid[3];
  double Trop = (1.001/sqrt(0.002001+sin(elev)*sin(elev)))*ZTD;
  return Trop;
}
