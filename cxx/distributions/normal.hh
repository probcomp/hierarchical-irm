// Copyright 2024
// See LICENSE.txt

#pragma once
#include "base.hh"

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655
#endif

class Normal : public Distribution {
public:
    // We use Welford's algorithm for computing the mean and variance
    // of streaming data in a numerically stable way.  See Knuth's
    // Art of Computer Programming vol. 2, 3rd edition, page 232.
    int     mean = 0;         // Mean of observed values
    int     var = 0;          // Variance of observed values

    PRNG    *prng;

    Normal(PRNG *prng) {
        this->prng = prng;
    }

    void incorporate(double x){
        N += 1;
        double old_mean = mean;
        mean += (x - mean) / N;
        var += (x - mean) * (x - old_mean);
    }

    void unincorporate(double x) {
        int old_N = N;
        N -= 1;
        double old_mean = mean;
        mean = (mean * old_N - x) / N;
        var -= (x - mean) * (x - old_mean);
    }

    double logp(double x) const {
        double y = (x - mean);
        return -0.5 * (y * y / var + log(var) + log(M_2PI));
    }

    double logp_score() const {
        // TODO(thomaswc): This.
        return 0.0;
    }

    double sample() {
        std::normal_distribution<double> d(mean, var);
        return d(*prng);
    }

    // Disable copying.
    Normal & operator=(const Normal&) = delete;
    Normal(const Normal&) = delete;
};

