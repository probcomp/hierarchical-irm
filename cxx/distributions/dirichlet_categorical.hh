// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>

#include "distributions/base.hh"
#include "util_math.hh"

#define ALPHA_GRID \
  { 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 10.0, 100.0, 1000.0, 10000.0 }

class DirichletCategorical : public Distribution<double> {
 public:
  double alpha = 1;         // hyperparameter (applies to all categories)
  std::vector<int> counts;  // counts of observed categories
  std::mt19937* prng;

  // DirichletCategorical does not take ownership of prng.
  DirichletCategorical(std::mt19937* prng,
                       int k) {  // k is number of categories
    this->prng = prng;
    counts = std::vector<int>(k, 0);
  }
  void incorporate(const double& x);

  void unincorporate(const double& x);

  double logp(const double& x) const;

  double logp_score() const;

  double sample();

  void transition_hyperparameters();

  DirichletCategorical* prior() const;
};
