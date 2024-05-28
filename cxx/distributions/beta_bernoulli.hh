// Copyright 2024
// See LICENSE.txt

#pragma once
#include <cassert>
#include "util_math.hh"
#include "distributions/base.hh"

class BetaBernoulli : public Distribution<double> {
public:
    double alpha = 1;  // hyperparameter
    double beta = 1;  // hyperparameter
    int s = 0;  // sum of observed values
    std::mt19937 *prng;

    std::vector<double> alpha_grid;
    std::vector<double> beta_grid;

    BetaBernoulli(std::mt19937 *prng) {
        this->prng = prng;
        alpha_grid = log_linspace(1e-4, 1e4, 10, true);
        beta_grid = log_linspace(1e-4, 1e4, 10, true);
    }
    void incorporate(const double& x){
        assert(x == 0 || x == 1);
        ++N;
        s += x;
    }
    void unincorporate(const double& x) {
        assert(x == 0 || x ==1);
        --N;
        s -= x;
        assert(0 <= s);
        assert(0 <= N);
    }
    double logp(const double& x) const {
        assert(x == 0 || x == 1);
        double log_denom = log(N + alpha + beta);
        double log_numer = x ? log(s + alpha) : log(N - s + beta);
        return log_numer - log_denom;
    }
    double logp_score() const {
        double v1 = lbeta(s + alpha, N - s + beta);
        double v2 = lbeta(alpha, beta);
        return v1 - v2;
    }
    double sample() {
        double p = exp(logp(1));
        std::vector<int> items {0, 1};
        std::vector<double> weights {1-p, p};
        int idx = choice(weights, prng);
        return items[idx];
    }
    void transition_hyperparameters() {
      std::vector<double> logps;
      for (alpha : alpha_grid) {
        for (beta : beta_grid) {
          logps.push_back(logp_score());
        }
      }
      int i = sample_from_logps(logps, prng);
      int N = beta_grid.size();
      alpha = alpha_grid[i / N];
      beta = beta_grid[i % N];
    }
};
