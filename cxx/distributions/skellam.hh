#pragma once

#include <cassert>
#include <cmath>

#include "distributions/nonconjugate.hh"
#include "util_math.hh"

#define MEAN_GRID { -10.0, 0.0, 10.0 }
#define STDDEV_GRID { 0.1, 1.0, 10.0 }

double lognormal_logp(double x, double mean, double stddev) {
  double y = (std::log(x) - mean) / stddev;
  return - y*y / 2.0
      - std::log(x * stddev) - 0.5 * std::log(2.0 * std::numbers::pi);
}

class Skellam : public NonconjugateDistribution<int> {
 public:
  // Skellam distribution with log Normal hyperprior of latent rates.
  double mean1, mean2, stddev1, stddev2;  // Hyperparameters
  // Skellam distribution with log-normal priors on mu1 and mu2.
  double mu1, mu2;   // Latent values.

  Skellam(): mean1(0.0), mean2(0.0), stddev1(1.0), stddev2(1.0),
             mu1(1.0), mu2(1.0) {}

  double logp(const int& x) const {
    return -mu1 - mu2 + (x / 2.0) * std::log(mu1 / mu2)
        // TODO(thomaswc): Replace this with something more numerically stable.
        + std::log(std::cyl_bessel_i(x, 2.0 * std::sqrt(mu1 * mu2)));
  }

  int sample(std::mt19937* prng) {
    std::poisson_distribution<int> d1(mu1);
    std::poisson_distribution<int> d2(mu2);
    return d1(*prng) - d2(*prng);
  }

  void transition_hyperparameters(std::mt19937* prng) {
    std::vector<double> logps;
    std::vector<std::tuple<double, double, double, double>> hypers;
    for (double tmean1 : MEAN_GRID) {
      for (double tstddev1 : STDDEV_GRID) {
        for (double tmean2 : MEAN_GRID) {
          for (double tstddev2 : STDDEV_GRID) {
            double lp = lognormal_logp(mu1, tmean1, tstddev1)
                        + lognormal_logp(mu2, tmean2, tstddev2);
            logps.push_back(lp);
            hypers.push_back(
                std::make_tuple(tmean1, tstddev1, tmean2, tstddev2));
          }
        }
      }
    }
    int i = sample_from_logps(logps, prng);
    mean1 = std::get<0>(hypers[i]);
    stddev1 = std::get<1>(hypers[i]);
    mean2 = std::get<2>(hypers[i]);
    stddev2 = std::get<3>(hypers[i]);
  }

  void init_theta(std::mt19937* prng) {
    std::normal_distribution<double> d1(mean1, stddev1);
    std::normal_distribution<double> d2(mean2, stddev2);
    mu1 = std::exp(d1(*prng));
    mu2 = std::exp(d2(*prng));
  }

  std::vector<double> store_latents() {
    std::vector<double> v;
    v.push_back(mu1);
    v.push_back(mu2);
    return v;
  }

  void set_latents(const std::vector<double>& v) {
    assert(v.size() == 2);
    mu1 = v[0];
    mu2 = v[1];
  }

};
