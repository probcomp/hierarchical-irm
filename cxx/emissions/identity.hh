#pragma once

#include "emissions/base.hh"

// An Emission class where dirty always equals clean.
template <typename SampleType = double>
class IdentityEmission : public Emission<SampleType> {
 public:
  IdentityEmission() {}
  ~IdentityEmission() {}

  void incorporate(const std::pair<SampleType, SampleType>& x,
                   double weight = 1.0) {
    this->N += weight;
  }

  double logp(const std::pair<SampleType, SampleType>& x) const {
    return 0.0;
  }

  double logp_score() const { return 0.0; }

  void transition_hyperparameters(std::mt19937* prng) {}

  SampleType sample_corrupted(const SampleType& clean, std::mt19937* prng) {
    return clean;
  }

  SampleType propose_clean(const std::vector<SampleType>& corrupted,
                           std::mt19937* prng) {
    return corrupted[0];
  }
};
