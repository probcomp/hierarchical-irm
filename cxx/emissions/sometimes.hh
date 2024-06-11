#pragma once

#include <unordered_map>

#include "distributions/beta_bernoulli.hh"
#include "emissions/base.hh"

// An Emission class that sometimes applies BaseEmissor and sometimes doesn't.
// BaseEmissor must (1) of type Emission<SampleType> and (2) assign zero
// probability to <clean, dirty> pairs with clean == dirty.  [For example,
// BitFlip and Gaussian both satisfy #2].
template <typename BaseEmissor, typename SampleType = double>
class Sometimes : public Emission<SampleType> {
 public:
  BetaBernoulli bb;
  BaseEmissor be;

  Sometimes() : bb(nullptr) {};

  void incorporate(const std::pair<SampleType, SampleType>& x) {
    ++N;
    bb.incorporate(x.first != x.second);
    if (x.first != x.second) {
      be.incorporate(x);
    }
  }

  void unincorporate(const std::pair<SampleType, SampleType>& x) {
    --N;
    bb.unincorporate(x.first != x.second);
    if (x.first != x.second) {
      be.unincorporate(x);
    }
  }

  double logp(const std::pair<SampleType, SampleType>& x) const {
    return bb.logp(x.first != x.second) + be.logp(x);
  }

  double logp_score() const {
    return bb.logp_score() + be.logp_score();
  }

  void transition_hyperparameters() {
    be.transition_hyperparameters();
    bb.transition_hyperparameters();
  }

  SampleType sample_corrupted(const SampleType& clean, std::mt19937* prng) {
    bb.prng = prng;
    if (bb.sample()) {
      return be.sample_corrupted(clean, prng);
    }
    return clean;
  }

  SampleType propose_clean(const std::vector<SampleType>& corrupted,
                           std::mt19937* prng) {
    // We approximate the maximum likelihood estimate by taking the mode of
    // corrupted.  The full solution would construct BaseEmissor and
    // BetaBernoulli instances for each choice of clean and picking the
    // clean with the lowest combined logp_score().
    std::unordered_map<SampleType, int> counts;
    SampleType mode;
    int max_count = 0;
    for (const SampleType& c: corrupted) {
      ++counts[c];
      if (counts[c] > max_count) {
        max_count = counts[c];
        mode = c;
      }
    }
    return mode;
  }
};
