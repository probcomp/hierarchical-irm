// Copyright 2024
// See LICENSE.txt

#include "zero_mean_normal.hh"

#include <cmath>
#include <numbers>

// Return log density of location-scaled T distribution with zero mean.
double log_t_distribution(const double& x, const double& v,
                          const double& variance) {
  // https://en.wikipedia.org/wiki/Student%27s_t-distribution#Density_and_first_two_moments
  double v_shift = (v + 1.0) / 2.0;
  return lgamma(v_shift)
      - lgamma(v / 2.0)
      - 0.5 * log(std::numbers::pi * v * variance)
      - v_shift * log1p(x * x / (variance * v));
}

void ZeroMeanNormal::incorporate(const double& x, double weight) {
  N += weight;
  if (N == 0.0) {
    var = 0.0;
    return;
  }
  var += weight * (x * x - var) / N;
}

double ZeroMeanNormal::logp(const double& x) const {
  // Equation (119) of https://www.cs.ubc.ca/~murphyk/Papers/bayesGauss.pdf
  double alpha_n = alpha + N / 2.0;
  double beta_n = beta + 0.5 * var * N;
  double t_variance = beta_n / alpha_n;
  return log_t_distribution(x, 2.0 * alpha_n, t_variance);
}

double ZeroMeanNormal::logp_score() const {
  // Marginal likelihood from Page 10 of
  // https://people.eecs.berkeley.edu/~jordan/courses/260-spring10/lectures/lecture5.pdf
  double alpha_n = alpha + N / 2.0;
  return alpha * log(beta)
      - lgamma(alpha)
      - (N / 2.0) * log(2.0 * std::numbers::pi)
      + lgamma(alpha_n)
      - alpha_n * log(beta + 0.5 * var * N);
}

double ZeroMeanNormal::sample(std::mt19937* prng) {
  double alpha_n = alpha + N / 2.0;
  double beta_n = beta + var * N;
  double t_variance = beta_n / alpha_n;
  std::student_t_distribution<double> d(2.0 * alpha_n);
  return d(*prng) * sqrt(t_variance);
}

#define ALPHA_GRID \
  { 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 10.0, 100.0, 1000.0, 10000.0 }
#define BETA_GRID \
  { 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 10.0, 100.0, 1000.0, 10000.0 }

void ZeroMeanNormal::transition_hyperparameters(std::mt19937* prng) {
  std::vector<double> logps;
  std::vector<std::pair<double, double>> hypers;
  for (double a : ALPHA_GRID) {
    for (double b : BETA_GRID) {
      alpha = a;
      beta = b;
      double lp = logp_score();
      if (!std::isnan(lp)) {
        logps.push_back(logp_score());
        hypers.push_back(std::make_pair(a, b));
      }
    }
  }

  int i = sample_from_logps(logps, prng);
  alpha = std::get<0>(hypers[i]);
  beta = std::get<1>(hypers[i]);
}
