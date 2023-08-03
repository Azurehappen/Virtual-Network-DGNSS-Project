#include "beidou_code_correction.h"

std::unordered_set<int> igso_prn = {6, 7, 8, 9, 10, 13, 16, 31, 38, 39, 40, 56};
std::unordered_set<int> meo_prn = {11, 12, 14, 19, 20, 21, 22, 23, 24, 25,
                                   26, 27, 28, 29, 30, 32, 33, 34, 35, 36,
                                   37, 41, 42, 43, 44, 45, 46, 57, 58};

std::unordered_map<int, std::vector<double>> igso = {
    {VN_CODE_BDS_C2I,
     {-0.55, -0.40, -0.34, -0.23, -0.15, -0.04, 0.09, 0.19, 0.27, 0.35}},
    {VN_CODE_BDS_C6I,
     {-0.71, -0.36, -0.33, -0.19, -0.14, -0.03, 0.08, 0.17, 0.24, 0.33}},
    {VN_CODE_BDS_C7,
     {-0.27, -0.23, -0.21, -0.15, -0.11, -0.04, 0.05, 0.14, 0.19, 0.32}}};

std::unordered_map<int, std::vector<double>> meo = {
    {VN_CODE_BDS_C2I,
     {-0.47, -0.38, -0.32, -0.23, -0.11, 0.06, 0.34, 0.69, 0.97, 1.05}},
    {VN_CODE_BDS_C6I,
     {-0.40, -0.31, -0.26, -0.18, -0.06, 0.09, 0.28, 0.48, 0.64, 0.69}},
    {VN_CODE_BDS_C7,
     {-0.22, -0.15, -0.13, -0.10, -0.04, 0.05, 0.14, 0.27, 0.36, 0.47}}};

double beidouCodeCorrection::computeBdsCodeCorr(int prn, double ele_deg,
                                                  int bds_code_type) {
  int tElev = floor(ele_deg / 10.0);
  double factor = fmod(ele_deg, 10.0) / 10.0;  // fraction for interpolation
  double corr = 0.0;

  std::unordered_map<int, std::vector<double>>* current_map = nullptr;

  if (igso_prn.find(prn) != igso_prn.end()) {
      current_map = &igso;
  } else if (meo_prn.find(prn) != meo_prn.end()) {
      current_map = &meo;
  }

  if (current_map && current_map->find(bds_code_type) != current_map->end()) {
      std::vector<double>& corrections = (*current_map)[bds_code_type];
      if (tElev < corrections.size() - 1) {
          double corr1 = corrections[tElev];
          double corr2 = corrections[tElev + 1];
          // Interpolation
          corr = corr1 + (corr2 - corr1) * factor;
      }
  }

  return corr;
}
