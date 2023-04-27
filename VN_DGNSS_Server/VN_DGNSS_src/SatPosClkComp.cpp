#include "SatPosClkComp.h"
#include <utility>

// earth gravitational constant
#define MU_GPS   3.9860050E14
#define MU_GAL   3.986004418E14
#define MU_BDS   3.986004418E14
// earth angular velocity (rad/s)
#define OMGE_GPS 7.2921151467E-5
#define OMGE_GAL 7.2921151467E-5
#define OMGE_BDS 7.292115E-5

#define SIN_5 -0.0871557427476582 /* sin(-5.0 deg) */
#define COS_5  0.9961946980917456 /* cos(-5.0 deg) */

static double MU_define(int sys) {
  if (sys == SYS_GPS) {
    return MU_GPS;
  } else if (sys == SYS_GAL) {
    return MU_GAL;
  } else if (sys == SYS_CMP) {
    return MU_BDS;
  } else {
    return 0.0;
  }
}

static double OMGE_define(int sys) {
  if (sys == SYS_GPS) {
    return OMGE_GPS;
  } else if (sys == SYS_GAL) {
    return OMGE_GAL;
  } else if (sys == SYS_CMP) {
    return OMGE_BDS;
  } else {
    return 0.0;
  }
}

static double F_define(int sys) {
  if (sys == SYS_GPS) {
    return -2*sqrt(MU_GPS)/(CLIGHT*CLIGHT);
  } else if (sys == SYS_GAL) {
    return -2*sqrt(MU_GAL)/(CLIGHT*CLIGHT);
  } else if (sys == SYS_CMP) {
    return -2*sqrt(MU_BDS)/(CLIGHT*CLIGHT);
  } else {
    return 0.0;
  }
}

SatPosClkComp::SatPosClkComp(gtime_t rcv_t, std::vector<double> dX, std::vector<double> dV,
                             gtime_t orb_corr_time, std::vector<double> dt_corr,
                             gtime_t clk_corr_time, satstruct::Ephemeris eph_data_0, int sys)
    : received_time(rcv_t),
      eph_0(eph_data_0),
      dX_rac(std::move(dX)),
      dV_rac(std::move(dV)),
      ssr_orbit_time(orb_corr_time),
      dt_corr(std::move(dt_corr)),
      ssr_clock_time(clk_corr_time),
      OmegaDot_e(OMGE_define(sys)),
      lambda_L1(0.190293672798365),
      lambda_L2(0.244210213424568),
      mu(MU_define(sys)),
      mask_ang_rad(0.2610),
      F(F_define(sys)),
      sat_pos_ecef(3, 0),
      sat_vel_ecef(3, 0),
      sat_pos_ecef_precise(3, 0),
      sat_pos_pre_trans(3, 0),
      sys(sys) {}

SatPosClkComp::~SatPosClkComp() = default;

double SatPosClkComp::limit_gpstime(double time_gps) {
  if (time_gps > 302400) {
    time_gps -= 604800;
  } else if (time_gps < -302400) {
    time_gps += 604800;
  }
  return time_gps;
}
void SatPosClkComp::SatClkComputation() {
  // transmit time with handover limited
  double t = timediff( transmit_time , eph_0.t_oc);
  // clock bias
  dt_clk = eph_0.a_f0 + eph_0.a_f1 * t + eph_0.a_f2 * t * t;
  // group delay L1
  dt_g1 = -eph_0.T_GD;
  // group delay L2
  double L1 = CLIGHT / lambda_L1;
  double L2 = CLIGHT / lambda_L2;
  dt_g2 = -(pow(L1 / L2, 2)) * eph_0.T_GD;
}
void SatPosClkComp::PreciseSatClkComputation() {
  double dt_p = timediff(transmit_time , ssr_clock_time);
  dt_clk_precise =
          (dt_corr[0] + dt_corr[1] * dt_p + dt_corr[2] * pow(dt_p, 2)) / CLIGHT;
}
double SatPosClkComp::KeplerAnomaly(double ecc, double M_k) {
  // set tolerance
  double tol = 1e-13;
  size_t max_iter = 25;
  double Ek = M_k;  // initial guess
  // improve guess with Newton-Raphson Iteration
  for (size_t i = 1; i <= max_iter; i++) {
    double Delta_E = (Ek - ecc * sin(Ek) - M_k) / (1 - ecc * cos(Ek));
    Ek -= Delta_E;
    if (std::abs(Delta_E) < tol) break;
  }
  return Ek;
}

void SatPosClkComp::SatEphPosComputation() {
  // semi-major axis (m)
  A = eph_0.sqrtA * eph_0.sqrtA;

  // time from ephemeris reference epoch
  // transmit time with handover limited
  double t_k = timediff(transmit_time , eph_0.t_oe);

  // computed mean motion (rad/s)
  double n_0 = sqrt(mu / (pow(A, 3)));

  // corrected mean motion
  n = n_0 + eph_0.Delta_n;

  // mean anomaly
  double M_k = eph_0.M_0 + n * t_k;
  E_k = KeplerAnomaly(eph_0.e, M_k);
  sEk = sin(E_k);
  cEk = cos(E_k);

  // true anomaly (rad)
  double v_k = atan2((sqrt(1 - pow(eph_0.e, 2)) * sEk) / (1 - eph_0.e * cEk),
                     (cEk - eph_0.e) / (1 - eph_0.e * cEk));
  cvk = cos(v_k);
  svk = sin(v_k);

  double Phi_k = v_k + eph_0.omega;
  // second harmonic perturbation
  s2Phik = sin(2 * Phi_k);
  c2Phik = cos(2 * Phi_k);
  double delta_u_k = eph_0.C_us * s2Phik + eph_0.C_uc * c2Phik;
  double delta_r_k = eph_0.C_rs * s2Phik + eph_0.C_rc * c2Phik;
  double delta_i_k = eph_0.C_is * s2Phik + eph_0.C_ic * c2Phik;

  double u_k = Phi_k + delta_u_k;
  cuk = cos(u_k);
  suk = sin(u_k);
  c2uk = cos(2 * u_k);
  s2uk = sin(2 * u_k);

  // corrected radius (m)
  r_k = A * (1 - eph_0.e * cEk) + delta_r_k;

  // corrected inclination (m)
  double i_k = eph_0.i_0 + delta_i_k + eph_0.IDOT * t_k;
  cik = cos(i_k);
  sik = sin(i_k);
  // Sv position in orbital plane after rotation thru argument of latitude (m)
  x_k_prime = r_k * cuk;
  y_k_prime = r_k * suk;
  if (sys == SYS_CMP && (eph_0.prn<=5 || eph_0.prn == 18)) {
    // Sat pos ocmputation for BeiDou GEO
    double Omega_k = eph_0.Omega_0 + eph_0.OmegaDot * t_k -
                     OmegaDot_e * eph_0.toes;
    sOmeg = sin(Omega_k);
    cOmeg = cos(Omega_k);
    double xg = x_k_prime * cOmeg - y_k_prime * cik * sOmeg;
    double yg = x_k_prime * sOmeg + y_k_prime * cik * cOmeg;
    double zg = y_k_prime * sik;
    double sino = sin(OmegaDot_e * t_k);
    double coso = cos(OmegaDot_e * t_k);
    x_k = xg*coso+yg*sino*COS_5+zg*sino*SIN_5;
    y_k = -xg*sino+yg*coso*COS_5+zg*coso*SIN_5;
    z_k = -yg*SIN_5+zg*COS_5;
  } else {
    // corrected longitude of ascending node (rad)
    double Omega_k = eph_0.Omega_0 + (eph_0.OmegaDot - OmegaDot_e) * t_k -
                     OmegaDot_e * eph_0.toes;
    sOmeg = sin(Omega_k);
    cOmeg = cos(Omega_k);

    // SV position in earth-fixed coordinates (ECEF)
    x_k = x_k_prime * cOmeg - y_k_prime * cik * sOmeg;
    y_k = x_k_prime * sOmeg + y_k_prime * cik * cOmeg;
    z_k = y_k_prime * sik;
  }

  // cout<<x_k<<" "<<y_k<<" "<<z_k<<endl;
  sat_pos_ecef[0] = x_k;
  sat_pos_ecef[1] = y_k;
  sat_pos_ecef[2] = z_k;

}

void SatPosClkComp::SatEphVelComputation() {
  double E_k_dot = n / (1 - eph_0.e * cEk);

  double Phi_k_dot =
          (sqrt(1 - pow(eph_0.e, 2)) / (1 - eph_0.e * cEk)) * E_k_dot;

  double u_k_dot =
          (1 + 2 * eph_0.C_us * c2Phik - 2 * eph_0.C_uc * s2Phik) * Phi_k_dot;

  double r_k_dot = 2 * (eph_0.C_rs * c2Phik - eph_0.C_rc * s2Phik) * Phi_k_dot +
                   A * eph_0.e * sEk * E_k_dot;

  double x_k_prime_dot = r_k_dot * cuk - r_k * suk * u_k_dot;
  double y_k_prime_dot = r_k_dot * suk + r_k * cuk * u_k_dot;

  double di_k_dt =
          2 * (eph_0.C_is * c2Phik - eph_0.C_ic * s2Phik) * Phi_k_dot + eph_0.IDOT;

  double Omega_k_dot = eph_0.OmegaDot - OmegaDot_e;

  double x_k_dot = x_k_prime_dot * cOmeg - y_k_prime_dot * cik * sOmeg +
                   y_k_prime * sik * sOmeg * di_k_dt - y_k * Omega_k_dot;
  double y_k_dot = x_k_prime_dot * sOmeg + y_k_prime_dot * cik * cOmeg -
                   y_k_prime * sik * cOmeg * di_k_dt + x_k * Omega_k_dot;
  double z_k_dot = y_k_prime_dot * sik + y_k_prime * cik * di_k_dt;

  sat_vel_ecef[0] = x_k_dot;
  sat_vel_ecef[1] = y_k_dot;
  sat_vel_ecef[2] = z_k_dot;
}

void SatPosClkComp::SatEphVelComputationUpdated() {
  double Mdot_k = n;
  double Edot_k = Mdot_k / (1 - eph_0.e * cEk);

  double vdot_k =
          sEk * Edot_k * (1 + eph_0.e * cvk) / (svk * (1 - eph_0.e * cEk));

  double udot_k = vdot_k + 2 * (eph_0.C_us * c2uk - eph_0.C_uc * s2uk) * vdot_k;

  double rdot_k = A * eph_0.e * sEk * n / (1 - eph_0.e * cEk) +
                  2 * (eph_0.C_rs * c2uk - eph_0.C_rc * s2uk) * vdot_k;

  double idot_k =
          eph_0.IDOT + (eph_0.C_is * c2uk - eph_0.C_ic * s2uk) * 2 * vdot_k;

  double xdot_k_prime = rdot_k * cuk - y_k_prime * udot_k;
  double ydot_k_prime = rdot_k * sik + x_k_prime * udot_k;

  double OmegaDot_k = eph_0.OmegaDot - OmegaDot_e;
  double xdot_k =
          (xdot_k_prime - y_k_prime * cik * OmegaDot_k) * cOmeg -
          (x_k_prime * OmegaDot_k + ydot_k_prime * cik - y_k_prime * sik * idot_k) *
          sOmeg;
  double ydot_k =
          (xdot_k_prime - y_k_prime * cik * OmegaDot_k) * sOmeg +
          (x_k_prime * OmegaDot_k + ydot_k_prime * cik - y_k_prime * sik * idot_k) *
          cOmeg;
  double zdot_k = ydot_k_prime * sik + y_k_prime * cik * idot_k;

  sat_vel_ecef[0] = xdot_k;
  sat_vel_ecef[1] = ydot_k;
  sat_vel_ecef[2] = zdot_k;
}
std::vector<double> SatPosClkComp::RAC2ECEF(std::vector<double> dp_RAC,
                                            std::vector<double> p_ecef,
                                            std::vector<double> v_ecef) {
  std::vector<double> unit_a(3, 0);
  double norm_a =
          sqrt(pow(v_ecef[0], 2) + pow(v_ecef[1], 2) + pow(v_ecef[2], 2));
  for (int i = 0; i < 3; i++) unit_a[i] = v_ecef[i] / norm_a;

  std::vector<double> unit_c(3, 0);
  std::vector<double> r(3, 0);
  r[0] = p_ecef[1] * v_ecef[2] - p_ecef[2] * v_ecef[1];
  r[1] = p_ecef[2] * v_ecef[0] - p_ecef[0] * v_ecef[2];
  r[2] = p_ecef[0] * v_ecef[1] - p_ecef[1] * v_ecef[0];
  double norm_r = sqrt(pow(r[0], 2) + pow(r[1], 2) + pow(r[2], 2));
  for (int i = 0; i < 3; i++) unit_c[i] = r[i] / norm_r;

  std::vector<double> unit_r(3, 0);
  unit_r[0] = unit_a[1] * unit_c[2] - unit_a[2] * unit_c[1];
  unit_r[1] = unit_a[2] * unit_c[0] - unit_a[0] * unit_c[2];
  unit_r[2] = unit_a[0] * unit_c[1] - unit_a[1] * unit_c[0];

  std::vector<std::vector<double> > ECEF_delta_mat(3, std::vector<double>(3, 0));
  for (int i = 0; i < 3; i++) {
    if (i == 0) {
      for (int j = 0; j < 3; j++) ECEF_delta_mat[j][i] = unit_r[j];
    }
    if (i == 1) {
      for (int j = 0; j < 3; j++) ECEF_delta_mat[j][i] = unit_a[j];
    }
    if (i == 2) {
      for (int j = 0; j < 3; j++) ECEF_delta_mat[j][i] = unit_c[j];
    }
  }
  std::vector<double> ECEF_delta_vector(3, 0);
  for (int i = 0; i < 3; i++) {
    double temp = 0;
    for (int j = 0; j < 3; j++) temp += ECEF_delta_mat[i][j] * dp_RAC[j];
    ECEF_delta_vector[i] = temp;
  }
  return ECEF_delta_vector;
}
void SatPosClkComp::PreciseSatPosComputation() {
  double dt = timediff(transmit_time , ssr_orbit_time);

  std::vector<double> dX_rac_n(3, 0);
  dX_rac_n[0] = dX_rac[0] + dV_rac[0] * dt;
  dX_rac_n[1] = dX_rac[1] + dV_rac[1] * dt;
  dX_rac_n[2] = dX_rac[2] + dV_rac[2] * dt;

  std::vector<double> dX_ecef = RAC2ECEF(dX_rac_n, sat_pos_ecef, sat_vel_ecef);
  sat_pos_ecef_precise[0] = sat_pos_ecef[0] - dX_ecef[0];
  sat_pos_ecef_precise[1] = sat_pos_ecef[1] - dX_ecef[1];
  sat_pos_ecef_precise[2] = sat_pos_ecef[2] - dX_ecef[2];

}

void SatPosClkComp::EarthRotationCorrection(std::vector<double> &x_pos) const {
  double theta = OMGE * propagation_time;
  double dx1 = cos(theta) * x_pos[0] + sin(theta) * x_pos[1];
  double dx2 = -sin(theta) * x_pos[0] + cos(theta) * x_pos[1];
  x_pos[0] = dx1;
  x_pos[1] = dx2;
}

double SatPosClkComp::Sagnac(std::vector<double> user_pos) {
  return OmegaDot_e *
         (sat_pos_ecef_precise[0] * user_pos[1] -
          sat_pos_ecef_precise[1] * user_pos[0]) /
         CLIGHT;
}

void SatPosClkComp::PropTimeOptm(std::vector<double> user_pos) {
  double tp_comp = 2.5 * pow(10, 7) / CLIGHT;
  // cout<<tp_comp<<endl;
  double h, dh_dt, tp_old;

  for (int i = 0; i < 20; i++) {
    // compute satellite position
    // step 1: compute time of transmit
    transmit_time = timeadd(received_time,-tp_comp);
    // step 2: compute satellite clock
    SatClkComputation();
    // step 3: precise compute satellite clock
    PreciseSatClkComputation();
    // step 4: compute transmit time
    transmit_time = timeadd(transmit_time,-dt_clk);
    // step 5: compute satellite position
    SatEphPosComputation();
    // step 6: compute satellite velocity
    SatEphVelComputationUpdated();
    dt_R = F * eph_0.e * eph_0.sqrtA * sin(E_k);
    dt_clk = dt_clk + dt_R;
    // step 7: compute precise satellite position
    PreciseSatPosComputation();
    dt_clk += dt_clk_precise;
    // filling in the satellite position computation function
    h = sqrt(pow(user_pos[0] - sat_pos_ecef_precise[0], 2) +
             pow(user_pos[1] - sat_pos_ecef_precise[1], 2) +
             pow(user_pos[2] - sat_pos_ecef_precise[2], 2));
    dh_dt =
            -(sat_vel_ecef[0] * (sat_pos_ecef_precise[0] - user_pos[0]) +
              sat_vel_ecef[1] * (sat_pos_ecef_precise[1] - user_pos[1]) +
              sat_vel_ecef[2] * (sat_pos_ecef_precise[2] - user_pos[2])) /
            h -
            OmegaDot_e / CLIGHT *
            (sat_vel_ecef[0] * user_pos[1] - sat_vel_ecef[1] * user_pos[0]) -
                CLIGHT;
    h = h + Sagnac(user_pos) - (tp_comp + dt_clk) * CLIGHT;
    tp_old = tp_comp;
    tp_comp = tp_comp - h / dh_dt;
    if (std::abs(tp_comp - tp_old) < 10e-11) break;
  }
  sat_pos_pre_trans = sat_pos_ecef_precise;
  // compute final propagaion and transmit time
  transmit_time = timeadd(received_time,-tp_comp);
  propagation_time = tp_comp + dt_clk;
}

void SatPosClkComp::ComputePreciseOrbitClockCorrection() {
  // rotation correction for satellite position
  EarthRotationCorrection(sat_pos_ecef);
  // rotation correction for precise satellite position
  EarthRotationCorrection(sat_pos_ecef_precise);
}

std::vector<double> SatPosClkComp::GetSatPos() { return sat_pos_ecef; }

std::vector<double> SatPosClkComp::GetPreciseSatPos() {
  return sat_pos_ecef_precise;
}
std::vector<double> SatPosClkComp::GetPreSatPosAtTranst() {
  return sat_pos_pre_trans;
}
double SatPosClkComp::GetClock() {
  return dt_clk  * CLIGHT;
}


