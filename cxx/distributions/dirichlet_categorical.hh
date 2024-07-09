// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>

#include "distributions/base.hh"
#include "util_math.hh"

#define ALPHA_GRID \
  { 1e-4, 1e-3, 1e-2, 1e-1, 1.0, 10.0, 100.0, 1000.0, 10000.0 }

class DirichletCategorical : public Distribution<int> {
 public:
  double alpha = 1;         // hyperparameter (applies to all categories)
  std::vector<double> counts;  // counts of observed categories

  DirichletCategorical(int k) {  // k is number of categories
    counts = std::vector<double>(k, 0.0);
  }
  void incorporate(const int& x, double weight = 1.0);

  double logp(const int& x) const;

  double logp_score() const;

  int sample(std::mt19937* prng);

  void transition_hyperparameters(std::mt19937* prng);
};
