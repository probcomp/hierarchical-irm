// Copyright 2024
// See LICENSE.txt

#include "distributions/beta_bernoulli.hh"

#include <cassert>

#include "util_math.hh"

void BetaBernoulli::incorporate(const bool& x, double weight) {
  assert(x == 0 || x == 1);
  N += weight;
  s += weight * x;
}

double BetaBernoulli::logp(const bool& x) const {
  assert(x == 0 || x == 1);
  double log_denom = log(N + alpha + beta);
  double log_numer = x ? log(s + alpha) : log(N - s + beta);
  return log_numer - log_denom;
}

double BetaBernoulli::logp_score() const {
  double v1 = lbeta(s + alpha, N - s + beta);
  double v2 = lbeta(alpha, beta);
  return v1 - v2;
}

bool BetaBernoulli::sample(std::mt19937* prng) {
  double p = exp(logp(1));
  std::vector<bool> items{false, true};
  std::vector<double> weights{1 - p, p};
  int idx = choice(weights, prng);
  return items[idx];
}

void BetaBernoulli::transition_hyperparameters(std::mt19937* prng) {
  std::vector<double> logps;
  std::vector<std::pair<double, double>> hypers;
  // C++ doesn't yet allow range for-loops over existing variables.  Sigh.
  for (double alphat : alpha_grid) {
    for (double betat : beta_grid) {
      alpha = alphat;
      beta = betat;
      double lp = logp_score();
      if (!std::isnan(lp)) {
        logps.push_back(logp_score());
        hypers.push_back(std::make_pair(alpha, beta));
      }
    }
  }
  if (logps.empty()) {
    printf("Warning! All hyperparamters for BetaBernoulli give nans!\n");
  } else {
    int i = sample_from_logps(logps, prng);
    alpha = hypers[i].first;
    beta = hypers[i].second;
  }
}
