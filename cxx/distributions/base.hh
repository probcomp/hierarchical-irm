#pragma once

template <typename SampleType = double> class Distribution {
// Abstract base class for probability distributions in HIRM.
public:
    // N is the number of incorporated observations.
    int N = 0;

    // Accumulate x.
    virtual void incorporate(const SampleType& x) = 0;

    // Undo the accumulation of x.  Should only be called with x's that
    // have been previously passed to incorporate().
    virtual void unincorporate(const SampleType& x) = 0;

    // The log probability of x according to the posterior predictive
    // distribution:  log P(x | incorporated_data), where P(x | data) =
    // \integral_{theta} P(x | theta ) P(theta | data) dtheta
    // and theta are the parameters of the distribution.
    virtual double logp(const SampleType& x) const = 0;

    // The log probability of the data we have accumulated so far according
    // to the prior:  log P(data | alpha) where alpha is the vector of
    // hyperparameters of the prior and P(data)
    // = \integral_{theta} P(data | theta) P(theta | alpha) dtheta.
    virtual double logp_score() const = 0;

    // A sample from the predictive distribution.
    // TODO(thomaswc): Consider refactoring so that this takes a
    // PRNG parameter.
    virtual SampleType sample() = 0;

    // Transition the hyperparameters.  The probability of transitioning to
    // a particular set of hyperparameters should be proportional to
    // e^logp_score() under those hyperparameters.
    virtual void transition_hyperparameters() = 0;

    virtual ~Distribution() = default;
};

