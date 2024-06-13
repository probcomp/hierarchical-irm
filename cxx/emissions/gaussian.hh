#pragma once

#include "emissions/base.hh"
#include "distributions/zero_mean_normal.hh"

class GaussianEmission : public Emission<double> {
 public:
  ZeroMeanNormal zmn(nullptr);

  GaussianEmission() {}

  void incorporate(const std::pair<double, double>& x) {
    ++N;
    zmn.incorporate(x.second - x.first);
  }

  void unincorporate(const std::pair<double, double>& x) {
    --N;
    zmn.unincorporate(x.second - x.first);
  }

  double logp(const std::pair<double, double>& x) const {
    return zmn.logp(x.second - x.first);
  }

  double logp_score() const {
    return zmn.logp_score();
  }

  double sample_corrupted(const double& clean, std::mt19937* prng) {
    zmn.prng = prng;
    return clean + zmn.sample();
  }

  double propose_clean(const std::vector<double>& corrupted,
                       std::mt19937* prng) {
    // The mean is the maximum likelihood estimate of the cleaned value.
    // We could also return a sample from a normal distribution centered at
    // mean.
    double mean = 0.0;
    int N = 0;
    for (auto const& c : corrupted) {
      N++;
      mean += (c - mean) / N;
    }
    return mean;
  }
}
