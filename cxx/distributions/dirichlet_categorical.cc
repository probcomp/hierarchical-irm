// Copyright 2024
// See LICENSE.txt

#include "distributions/dirichlet_categorical.hh"

#include <algorithm>
#include <cassert>
#include <random>

#include "util_math.hh"

void DirichletCategorical::incorporate(const double& x) {
  assert(x >= 0 && x < counts.size());
  counts[size_t(x)] += 1;
  ++N;
}

void DirichletCategorical::unincorporate(const double& x) {
  const size_t y = x;
  assert(y < counts.size());
  counts[y] -= 1;
  --N;
  assert(0 <= counts[y]);
  assert(0 <= N);
}

double DirichletCategorical::logp(const double& x) const {
  assert(x >= 0 && x < counts.size());
  const double numer = log(alpha + counts[size_t(x)]);
  const double denom = log(N + alpha * counts.size());
  return numer - denom;
}

double DirichletCategorical::logp_score() const {
  const size_t k = counts.size();
  const double a = alpha * k;
  double lg = 0;
  for (size_t x : counts) {
    lg += lgamma(static_cast<double>(x) + alpha);
  }
  return lgamma(a) - lgamma(a + N) + lg - k * lgamma(alpha);
}
double DirichletCategorical::sample() {
  std::vector<double> weights(counts.size());
  std::transform(counts.begin(), counts.end(), weights.begin(),
                 [&](size_t y) -> double { return y + alpha; });
  int idx = choice(weights, prng);
  return double(idx);
}

void DirichletCategorical::transition_hyperparameters() {
  std::vector<double> logps;
  std::vector<double> alphas;
  // C++ doesn't yet allow range for-loops over existing variables.  Sigh.
  for (double alphat : ALPHA_GRID) {
    alpha = alphat;
    double lp = logp_score();
    if (!std::isnan(lp)) {
      logps.push_back(logp_score());
      alphas.push_back(alpha);
    }
  }
  int i = sample_from_logps(logps, prng);
  alpha = alphas[i];
}
