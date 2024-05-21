// Copyright 2024
// See LICENSE.txt

#pragma once
#include "base.hh"

class BetaBernoulli : public Distribution<double> {
public:
    double  alpha = 1;       // hyperparameter
    double  beta = 1;        // hyperparameter
    int     s = 0;           // sum of observed values
    PRNG    *prng;

    BetaBernoulli(PRNG *prng) {
        this->prng = prng;
    }
    void incorporate(double x){
        assert(x == 0 || x == 1);
        N += 1;
        s += x;
    }
    void unincorporate(double x) {
        assert(x == 0 || x ==1);
        N -= 1;
        s -= x;
        assert(0 <= s);
        assert(0 <= N);
    }
    double logp(double x) const {
        double log_denom = log(N + alpha + beta);
        if (x == 1) { return log(s + alpha) - log_denom; }
        if (x == 0) { return log(N - s + beta) - log_denom; }
        assert(false);
    }
    double logp_score() const {
        double v1 = lbeta(s + alpha, N - s + beta);
        double v2 = lbeta(alpha, beta);
        return v1 - v2;
    }
    double sample() {
        double p = exp(logp(1));
        vector<int> items {0, 1};
        vector<double> weights {1-p, p};
        auto idx = choice(weights, prng);
        return items[idx];
    }

    // Disable copying.
    BetaBernoulli & operator=(const BetaBernoulli&) = delete;
    BetaBernoulli(const BetaBernoulli&) = delete;
};
