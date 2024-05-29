// Copyright 2024
// See LICENSE.txt

#include <cmath>
#include <numbers>

#include "normal.hh"

double logZ(double r, double v, double s) {
  return (v + 1.0) / 2.0 * log(2.0)
      + 0.5 * log(std::numbers::pi)
      - 0.5 * log(r)
      - 0.5 * v * log(s)
      + lgamma(0.5 * v);
}

void Normal::incorporate(const double &x){
    ++N;
    double old_mean = mean;
    mean += (x - mean) / N;
    var += (x - mean) * (x - old_mean);
}

void Normal::unincorporate(const double &x) {
    int old_N = N;
    --N;
    if (N == 0) {
      mean = 0.0;
      var = 0.0;
      return;
    }
    double old_mean = mean;
    mean = (mean * old_N - x) / N;
    var -= (x - mean) * (x - old_mean);
}

void Normal::posterior_hypers(double *mprime, double *sprime) const {
  // r' = r + N
  // m' = (r m + N mean) / (r + N)
  // C = N (var + mean^2)
  // s' = s + C + r m^2 - r' m' m'
  double mdelta = r * (m - mean) / (r + N);
  *mprime = mean + mdelta;
  *sprime = s + r * (m * m - *mprime * *mprime) +
            N * (var - 2 * mean * mdelta - mdelta * mdelta);
}

double Normal::logp(const double& x) const {
  // Based on equation (13) of GaussianInverseGamma.pdf
  double unused_mprime, sprime;
  const_cast<Normal*>(this)->incorporate(x);
  posterior_hypers(&unused_mprime, &sprime);
  const_cast<Normal*>(this)->unincorporate(x);
  double sprime2;
  posterior_hypers(&unused_mprime, &sprime2);
  return -0.5 * log(M_2PI)
      + logZ(r + N + 1, v + N + 1, sprime)
      - logZ(r + N, v + N, sprime2);
}

double Normal::logp_score() const {
  // Based on equation (11) of GaussianInverseGamma.pdf
  double unused_mprime, sprime;
  posterior_hypers(&unused_mprime, &sprime);
  return -0.5 * N * log(M_2PI)
      + logZ(r + N, v + N, sprime)
      - logZ(r, v, s);
}

double Normal::sample() {
  double rn = r + N;
  double nu = v + N;
  double mprime, sprime;
  posterior_hypers(&mprime, &sprime);
  std::gamma_distribution<double> rho_dist(nu / 2.0, 2.0 / sprime);
  double rho = rho_dist(*prng);
  std::normal_distribution<double> mean_dist(mprime, 1.0 / sqrt(rho * rn));
  double smean = mean_dist(*prng);
  double std_dev = 1.0 / sqrt(rho);

  std::normal_distribution<double> d(smean, std_dev);
  return d(*prng);
}

void Normal::transition_hyperparameters() {
  std::vector<double> logps;
  std::vector<std::tuple<double, double, double, double>> hypers;
  for (double rt : R_GRID) {
    for (double vt : V_GRID) {
      for (double mt : M_GRID) {
        for (double st : S_GRID) {
          r = rt;
          v = vt;
          m = mt;
          s = st;
          double lp = logp_score();
          if (!std::isnan(lp)) {
            logps.push_back(logp_score());
            hypers.push_back(std::make_tuple(r, v, m, s));
          }
        }
      }
    }
  }

  int i = sample_from_logps(logps, prng);
  r = std::get<0>(hypers[i]);
  v = std::get<1>(hypers[i]);
  m = std::get<2>(hypers[i]);
  s = std::get<3>(hypers[i]);
}
