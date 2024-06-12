// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>
#include <tuple>
#include <variant>

#include "base.hh"
#include "util_math.hh"

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655
#endif

#define R_GRID \
  { 0.1, 1.0, 10.0 }
#define V_GRID \
  { 0.5, 1.0, 2.0, 5.0 }
#define M_GRID \
  { -1.0, 0.0, 1.0 }
#define S_GRID \
  { 0.5, 1.0, 2.0 }

double logZ(double r, double v, double s);

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
  double mean = 0.0;  // Mean of observed values
  double var = 0.0;   // Variance of observed values

  std::mt19937* prng;

  // Normal does not take ownership of prng.
  Normal(std::mt19937* prng) { this->prng = prng; }

  void incorporate(const double& x);

  void unincorporate(const double& x);

  void posterior_hypers(double* mprime, double* sprime) const;

  double logp(const double& x) const;

  double logp_score() const;

  double sample();

  void transition_hyperparameters();

  // Disable copying.
  Normal& operator=(const Normal&) = delete;
  Normal(const Normal&) = delete;
};
