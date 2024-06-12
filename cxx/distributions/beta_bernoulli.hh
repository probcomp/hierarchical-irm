// Copyright 2024
// See LICENSE.txt

#pragma once
#include <cassert>

#include "distributions/base.hh"
#include "util_math.hh"

// TODO(thomaswc, emilyaf): Change BetaBernoulli to use bool instead of
// double.
class BetaBernoulli : public Distribution<double> {
 public:
  double alpha = 1;  // hyperparameter
  double beta = 1;   // hyperparameter
  int s = 0;         // sum of observed values
  std::mt19937* prng;

  std::vector<double> alpha_grid;
  std::vector<double> beta_grid;

  // BetaBernoulli does not take ownership of prng.
  BetaBernoulli(std::mt19937* prng) {
    this->prng = prng;
    alpha_grid = log_linspace(1e-4, 1e4, 10, true);
    beta_grid = log_linspace(1e-4, 1e4, 10, true);
  }

  void incorporate(const double& x);

  void unincorporate(const double& x);

  double logp(const double& x) const;

  double logp_score() const;

  double sample();

  void transition_hyperparameters();
};
