// Copyright 2024
// See LICENSE.txt

#pragma once
#include "base.hh"

#ifndef M_2PI
#define M_2PI 6.28318530717958647692528676655
#endif

class Normal : public Distribution<double> {
public:
    // Hyperparameters:
    // The conjugate prior to a normal distribution is a
    // normal-inverse-gamma distribution, which we parameterize following
    // https://en.wikipedia.org/wiki/Normal-inverse-gamma_distribution .
    double mu = 0;
    double lambda = 1.0;
    double alpha = 1.0;
    double beta = 1.0;

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

    // P(x | data)
    // = \int_{mean, var} P(x | mean, var) P(mean, var | data) dmean dvar
    // = \int_{mean, var} N(x | mean, var) P(data | mean, var) P(mean, var) /
    // P(data) dmean dvar
    // = \int_{mean, var} \prod_i N( data_i | mean, var) Prior(mean, var) /
    // P(data) dmean dvar
    // with the convention that data_0 = x.
    double logp(double x) const {
        double y = (x - mean);
        return -0.5 * (y * y / var + log(var) + log(M_2PI));
    }

    double logp_score() const {
        // Based on
        // https://en.wikipedia.org/wiki/Normal-inverse-gamma_distribution#Probability_density_function
        double y = mean - mu;
        return 0.5 * log(lambda) - 0.5 * log(var) - 0.5 * log(M_2PI)
               - alpha * log(beta) - lgamma(alpha) - (alpha + 1) * log(var)
               - (2 * beta + lambda * y * y) / (2.0 * var);
    }

    double sample() {
        std::normal_distribution<double> d(mean, var);
        return d(*prng);
    }

    // Disable copying.
    Normal & operator=(const Normal&) = delete;
    Normal(const Normal&) = delete;
};

