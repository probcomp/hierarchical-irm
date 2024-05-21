// Copyright 2024
// See LICENSE.txt

#pragma once
#include "globals.hh"
#include "distributions/base.hh"

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
    int     mean = 0;         // Mean of observed values
    int     var = 0;          // Variance of observed values, 1 / precision.

    PRNG    *prng;

    Normal(PRNG *prng) {
        this->prng = prng;
    }

    void incorporate(double x){
        N += 1;
        double old_mean = mean;
        mean += (x - mean) / N;
        var += (x - mean) * (x - old_mean);
    }

    void unincorporate(double x) {
        int old_N = N;
        N -= 1;
        double old_mean = mean;
        mean = (mean * old_N - x) / N;
        var -= (x - mean) * (x - old_mean);
    }

    double sprime() const {
      // r' = r + N
      // m' = (r m + N mean) / (r + N)
      // C = N (var + mean^2)
      // s' = s + C + r m^2 - r' m' m'
      double mdelta = r * (m - mean) / (r + N);
      double mprime = mean + mdelta;
      return s + r * (m * m - mprime * mprime)
          + N * (var - 2 * mean * mdelta - mdelta * mdelta);
    }

    double logp(double x) const {
      return -0.5 * N * log(M_2PI)
          + logZ(r + N, v + N, sprime())
          - logZ(r, v, s);
    }

    double logp_score() const {
      double sp = sprime();
      double mdelta = r * (m - mean) / (r + N);
      return -logZ(r + N, v + N, sp)
          - 0.5 * (v + N - 1) * log(var)
          - 0.5 * ((r + N) * mdelta * mdelta + sp)/ var;
    }

    double sample() {
      // TODO(thomaswc): These should actually be samples from the predictive
      // distribution.
      std::normal_distribution<double> d(mean, var);
      return d(*prng);
    }

    // Disable copying.
    Normal & operator=(const Normal&) = delete;
    Normal(const Normal&) = delete;
};

