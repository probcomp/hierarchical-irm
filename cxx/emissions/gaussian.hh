#pragma once

#include "emissions/base.hh"

class GaussianEmission : public Emission<double> {
 public:
  // We use an Inverse gamma conjugate prior, so our hyperparameters are
  double alpha = 1.0;
  double beta = 1.0;

  // Sufficient statistics of observed data.
  double var = 0.0;

  GaussianEmission() {}

  void incorporate(const std::pair<double, double> &x) {
    ++N;
    double y = x.second - x.first;
    var += y * y;
  }

  void unincorporate(const std::pair<double, double> &x) {
    --N;
    double y = x.second - x.first;
    var -= y * y;
  }

  double logp(const std::pair<double, double> &x) const {
    double y = x.second - x.first;
    // TODO(thomaswc): This.
    return 0.0;
  }

  double logp_score() const {
    // TODO(thomaswc): This.
    return 0.0;
  }

  double sample_corrupted(const double& clean, std::mt19937 *prng) {
    double smoothed_var = var;  // TODO(thomaswc): Fix
    std::normal_distribution<double> d(clean, sqrt(smoothed_var));
    return d(*prng);
  }

  double propose_clean(const std::vector<double> &corrupted,
                       std::mt19937 *prng) {
    // The mean is the maximum likelihood estimate of the cleaned value.
    // We could also return a sample from a normal distribution centered at
    // mean.
    double mean = 0.0;
    int N = 0;
    for (auto const &c : corrupted) {
      N++;
      mean += (c - mean) / N;
    }
    return mean;
  }
}
