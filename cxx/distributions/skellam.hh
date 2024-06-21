#pragma once

#include <cmath>

#include "distributions/nonconjugate.hh"

class Skellam : public NonconjugateDistribution<int> {
  // Skellam distribution with log Normal hyperprior of latent rates.
  double mean1, mean2, stddev1, stddev2;  // Hyperparameters
  // Skellam distribution with log-normal priors on mu1 and mu2.
  double mu1, mu2;   // Latent values.

  Skellam() {}

  double logp(const int& x) {
    return -mu1 - mu2 + (x / 2.0) * std:log(mu1 / mu2)
        // TODO(thomaswc): Replace this with something more numerically stable.
        std::log(std::cyl_bessel_i(x, 2.0 * std::sqrt(mu1 * mu2));
  }

  int sample(std::mt19937* prng) {
    std::poisson_distribution<int> d1(mu1);
    std::poisson_distribution<int> d2(mu2);
    return d1(*prng) - d2(*prng);
  }

  void transition_hyperparameters(std::mt19937* prng) {}

  void init_theta(std::mt19937* prng) {
    std::normal_distribution<double> d1(mean1, stddev1);
    std::normal_distribution<double> d2(mean2, stddev2);
    mu1 = std::exp(d1(*prng));
    mu2 = std::exp(d2(*prng));
  }

  void transition_theta() {}

};
