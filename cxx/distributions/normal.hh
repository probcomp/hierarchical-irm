// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>
#include "base.hh"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419716939937510
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655
#endif

double logZ(double r, double v, double s) {
  return (v + 1.0) / 2.0 * log(2.0)
      + 0.5 * log(M_PI)
      - 0.5 * log(r)
      - 0.5 * v * log(s)
      + lgamma(0.5 * v);
}

class Normal : public Distribution<double> {
public:
    // Hyperparameters:
    // The conjugate prior to a normal distribution is a
    // normal-inverse-gamma distribution, which we parameterize following
    // http://www.stats.ox.ac.uk/~teh/research/notes/GaussianInverseGamma.pdf
    double r = 1.0;  // Relative precision of mean versus data.
    double v = 1.0;  // Degrees of freedom of precision.
    double m = 0.0;  // Mean of mean's distribution.
    double s = 1.0;  // Mean of precision is v/s.

    // We use Welford's algorithm for computing the mean and variance
    // of streaming data in a numerically stable way.  See Knuth's
    // Art of Computer Programming vol. 2, 3rd edition, page 232.
    int mean = 0;         // Mean of observed values
    int var = 0;          // Variance of observed values

    std::mt19937 *prng;

    Normal(std::mt19937 *prng) {
        this->prng = prng;
    }

    void incorporate(const double& x){
        ++N;
        double old_mean = mean;
        mean += (x - mean) / N;
        var += (x - mean) * (x - old_mean);
    }

    void unincorporate(const double& x) {
        int old_N = N;
        --N;
        double old_mean = mean;
        mean = (mean * old_N - x) / N;
        var -= (x - mean) * (x - old_mean);
    }

    void posterior_hypers(double *mprime, double *sprime) const {
      // r' = r + N
      // m' = (r m + N mean) / (r + N)
      // C = N (var + mean^2)
      // s' = s + C + r m^2 - r' m' m'
      double mdelta = r * (m - mean) / (r + N);
      *mprime = mean + mdelta;
      *sprime = s + r * (m * m - *mprime * *mprime)
          + N * (var - 2 * mean * mdelta - mdelta * mdelta);
    }

    double logp(const double& x) const {
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

    double logp_score() const {
      // Based on equation (11) of GaussianInverseGamma.pdf
      double unused_mprime, sprime;
      posterior_hypers(&unused_mprime, &sprime);
      return -0.5 * N * log(M_2PI)
          + logZ(r + N, v + N, sprime)
          - logZ(r, v, s);
    }

    double sample() {
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

    void transition_hyperparameters() {
      // TODO(thomaswc): Implement this.
    }

    // Disable copying.
    Normal & operator=(const Normal&) = delete;
    Normal(const Normal&) = delete;
};

