#pragma once

class Distribution {
// Abstract base class for probability distributions in HIRM.
// These probability distributions are generative, and so must come with
// a prior (usually a conjugate prior) over the parameters implied by
// their observed data.
public:
    // N is the number of incorporated observations.
    int N = 0;

    // Accumulate x.
    virtual void incorporate(double x) = 0;

    // Undo the accumulation of x.  Should only be called with x's that
    // have been previously passed to incorporate().
    virtual void unincorporate(double x) = 0;

    // The log probability of x according to the distribution we have
    // accumulated so far.
    virtual double logp(double x) const = 0;

    // The log probability of the data we have accumulated so far according
    // to the prior.
    virtual double logp_score() const = 0;

    // A sample from the distribution we have accumulated so far.
    // TODO(thomaswc): Consider refactoring so that this takes a
    // PRNG parameter.
    virtual double sample() = 0;

    ~Distribution(){};
};

