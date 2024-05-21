#pragma once

template <typename SampleType = double> class Distribution {
// Abstract base class for probability distributions in HIRM.
public:
    // N is the number of incorporated observations.
    int N = 0;

    virtual void incorporate(SampleType x) = 0;
    virtual void unincorporate(SampleType x) = 0;

    // The log probability of x according to the distribution we have
    // accumulated so far.
    virtual double logp(SampleType x) const = 0;

    virtual double logp_score() const = 0;

    // A sample from the distribution we have accumulated so far.
    virtual SampleType sample() = 0;

    virtual ~Distribution() = default;
};

