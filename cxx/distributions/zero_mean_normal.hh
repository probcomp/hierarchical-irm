// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>
#include <tuple>
#include <variant>

#include "base.hh"
#include "util_math.hh"

// A normal distribution that is known to have zero mean.
// This class is not intended to be directly used as a data model, but
// rather to support the Gaussian emissions model.
class ZeroMeanNormal : public Distribution<double> {
 public:
  // We use an Inverse gamma conjugate prior, so our hyperparameters are
  double alpha = 1.0;
  double beta = 1.0;

  // Sufficient statistics of observed data.
  double var = 0.0;

  ZeroMeanNormal() {}

  void incorporate(const double& x, double weight = 1.0);

  void posterior_hypers(double* mprime, double* sprime) const;

  double logp(const double& x) const;

  double logp_score() const;

  double sample(std::mt19937* prng);

  void transition_hyperparameters(std::mt19937* prng);

  // Disable copying.
  ZeroMeanNormal& operator=(const ZeroMeanNormal&) = delete;
  ZeroMeanNormal(const ZeroMeanNormal&) = delete;
};
