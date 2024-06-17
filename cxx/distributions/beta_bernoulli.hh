// Copyright 2024
// See LICENSE.txt

#pragma once
#include <cassert>

#include "distributions/base.hh"
#include "util_math.hh"

class BetaBernoulli : public Distribution<bool> {
 public:
  double alpha = 1;  // hyperparameter
  double beta = 1;   // hyperparameter
  int s = 0;         // sum of observed values

  std::vector<double> alpha_grid;
  std::vector<double> beta_grid;

  BetaBernoulli() {
    alpha_grid = log_linspace(1e-4, 1e4, 10, true);
    beta_grid = log_linspace(1e-4, 1e4, 10, true);
  }

  void incorporate(const bool& x);

  void unincorporate(const bool& x);

  double logp(const bool& x) const;

  double logp_score() const;

  bool sample(std::mt19937* prng);

  void transition_hyperparameters(std::mt19937* prng);
};
