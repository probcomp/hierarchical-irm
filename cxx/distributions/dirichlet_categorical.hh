// Copyright 2024
// See LICENSE.txt

#pragma once
#include <algorithm>
#include <cassert>
#include <random>

#include "base.hh"
#include "util_math.hh"

class DirichletCategorical : public Distribution<double> {
 public:
  double alpha = 1;         // hyperparameter (applies to all categories)
  std::vector<int> counts;  // counts of observed categories
  int n;                    // Total number of observations.
  std::mt19937* prng;

  // DirichletCategorical does not take ownership of prng.
  DirichletCategorical(std::mt19937* prng,
                       int k) {  // k is number of categories
    this->prng = prng;
    counts = std::vector<int>(k, 0);
    n = 0;
  }
  void incorporate(const double& x) {
    assert(x >= 0 && x < counts.size());
    counts[size_t(x)] += 1;
    ++n;
  }
  void unincorporate(const double& x) {
    const size_t y = x;
    assert(y < counts.size());
    counts[y] -= 1;
    --n;
    assert(0 <= counts[y]);
    assert(0 <= n);
  }
  double logp(const double& x) const {
    assert(x >= 0 && x < counts.size());
    const double numer = log(alpha + counts[size_t(x)]);
    const double denom = log(n + alpha * counts.size());
    return numer - denom;
  }
  double logp_score() const {
    const size_t k = counts.size();
    const double a = alpha * k;
    const double lg = std::transform_reduce(
        counts.cbegin(), counts.cend(), 0, std::plus{},
        [&](size_t y) -> double { return lgamma(y + alpha); });
    return lgamma(a) - lgamma(a + n) + lg - k * lgamma(alpha);
  }
  double sample() {
    std::vector<double> weights(counts.size());
    std::transform(counts.begin(), counts.end(), weights.begin(),
                   [&](size_t y) -> double { return y + alpha; });
    int idx = choice(weights, prng);
    return double(idx);
  }
};
