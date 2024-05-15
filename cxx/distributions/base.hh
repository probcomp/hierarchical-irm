#pragma once

class Distribution {
// Abstract base class for probability distributions in HIRM.
public:
    // N is the number of incorporated observations.
    int N = 0;

    virtual void incorporate(double x) = 0;
    virtual void unincorporate(double x) = 0;

    // The log probability of x according to the distribution we have
    // accumulated so far.
    virtual double logp(double x) const = 0;

    virtual double logp_score() const = 0;

    // A sample from the distribution we have accumulated so far.
    virtual double sample() = 0;

    ~Distribution(){};
};

