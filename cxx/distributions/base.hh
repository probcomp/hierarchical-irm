#pragma once
#include<random>

template <typename T>
class Distribution {
  // Abstract base class for probability distributions in HIRM.
  // New distribution subclasses need to be added to
  // `util_distribution_variant` to be used in the (H)IRM models.
 public:
  typedef T SampleType;
  // N is the sum of the weights of the incorporated observations.
  double N = 0;

  // Accumulate x.
  virtual void incorporate(const T& x, double weight = 1.0) = 0;

  // Undo the accumulation of x.  Should only be called with x's that
  // have been previously passed to incorporate().
  virtual void unincorporate(const T& x) {
    incorporate(x, -1.0);
  }

  // The log probability of x according to the posterior predictive
  // distribution:  log P(x | incorporated_data), where P(x | data) =
  // \integral_{theta} P(x | theta ) P(theta | data) dtheta
  // and theta are the parameters of the distribution.
  virtual double logp(const T& x) const = 0;

  // The log probability of the data we have accumulated so far according
  // to the prior:  log P(data | alpha) where alpha is the vector of
  // hyperparameters of the prior and P(data)
  // = \integral_{theta} P(data | theta) P(theta | alpha) dtheta.
  virtual double logp_score() const = 0;

  // A sample from the predictive distribution.
  virtual T sample(std::mt19937* prng) = 0;

  // Transition the hyperparameters.  The probability of transitioning to
  // a particular set of hyperparameters should be proportional to
  // e^logp_score() under those hyperparameters.
  virtual void transition_hyperparameters(std::mt19937* prng) = 0;

  // Set the current latent values to a sample from the parameter prior.
  // Only children of NonconjugateDistribution need define this.
  virtual void init_theta(std::mt19937* prng) {};

  // Transition the current latent values.  Only children of
  // NonconjugateDistribution need define this.
  virtual void transition_theta(std::mt19937* prng) {};

  virtual ~Distribution() = default;
};
