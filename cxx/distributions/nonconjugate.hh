#pragma once

#include <map>
#include <random>
#include "distributions/base.hh"

template <typename T>
class NonconjugateDistribution : public Distribution<T> {
 public:
  // Abstract base class for Distributions that don't have conjugate priors.
  std::map<T, int> seen;

  // The log probability of x given the current latent values.
  virtual double logp(const T& x) const = 0;

  // Sample a value from the distribution given the current latent values.
  virtual T sample(std::mt19937* prng) = 0;

  // Transition hyperparameters given the current latent values.
  virtual void transition_hyperparameters(std::mt19937* prng) = 0;

  // Set the current latent values to a sample from the parameter prior.
  virtual void init_theta(std::mt19937* prng) = 0;

  // Return the current latent values as a vector.
  virtual std::vector<double> store_latents() = 0;

  // Set the current latent values from a vector.
  virtual void set_latents(const std::vector<double>& v) = 0;

  void incorporate(const T& x) {
    seen[x]++;
    (this->N)++;
  };

  void unincorporate(const T& x) {
    --seen[x];
    --(this->N);
  };

  double logp_score() const {
    double score = 0.0;
    for (const auto &it : seen) {
      score += logp(it.first) * it.second;;
    }
    return score;
  }

  // Transition the current latent values using Metropolis-Hastings.
  // Children classes are welcome to replace this with something more
  // powerful (like Hamiltonian Monte Carlo) if they like.
  virtual void transition_theta(std::mt19937* prng) {
    std::vector<double> old_latents = store_latents();
    double old_logp_score = logp_score();
    init_theta();
    double new_logp_score = logp_score();
    double threshold = std::exp(new_logp_score - old_logp_score);
    std::uniform_real_distribution rnd(0.0, 1.0);
    double p = rnd(*prng);
    if (p <= threshold) {
      // Accept
      return;
    }
    // Reject
    set_latents(old_latents);
  }

};
