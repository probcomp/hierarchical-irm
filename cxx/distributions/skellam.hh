#pragma once

#include "distributions/nonconjugate.hh"

#define MEAN_GRID { -10.0, 0.0, 10.0 }
#define STDDEV_GRID { 0.1, 1.0, 10.0 }

class Skellam : public NonconjugateDistribution<int> {
 public:
  // Skellam distribution with log Normal hyperprior of latent rates.
  double mean1, mean2, stddev1, stddev2;  // Hyperparameters
  // Skellam distribution with log-normal priors on mu1 and mu2.
  double mu1, mu2;   // Latent values.

  Skellam(): mean1(0.0), mean2(0.0), stddev1(1.0), stddev2(1.0),
             mu1(1.0), mu2(1.0) {}

  double logp(const int& x) const;

  int sample(std::mt19937* prng);

  void transition_hyperparameters(std::mt19937* prng);

  void init_theta(std::mt19937* prng);

  std::vector<double> store_latents() const;

  void set_latents(const std::vector<double>& v);
};
