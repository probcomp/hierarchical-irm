#pragma once

#include <unordered_map>

#include "distributions/beta_bernoulli.hh"
#include "emissions/base.hh"

// An Emission class that sometimes applies BaseEmissor and sometimes doesn't.
// BaseEmissor must assign zero probability to <clean, dirty> pairs with
// clean == dirty.  [For example, BitFlip and Gaussian both satisfy this].
template <typename BaseEmissor>
class Sometimes : public Emission<typename std::tuple_element<
                      0, typename BaseEmissor::SampleType>::type> {
 public:
  using SampleType =
      typename std::tuple_element<0, typename BaseEmissor::SampleType>::type;
  BetaBernoulli bb;
  BaseEmissor be;

  Sometimes(){};

  void incorporate(const std::pair<SampleType, SampleType>& x,
                   double weight = 1.0) {
    this->N += weight;
    bb.incorporate(x.first != x.second, weight);
    if (x.first != x.second) {
      be.incorporate(x, weight);
    }
  }

  double logp(const std::pair<SampleType, SampleType>& x) const {
    if (x.first != x.second) {
      return bb.logp(true) + be.logp(x);
    }
    return bb.logp(false);
  }

  double logp_score() const { return bb.logp_score() + be.logp_score(); }

  void transition_hyperparameters(std::mt19937* prng) {
    be.transition_hyperparameters(prng);
    bb.transition_hyperparameters(prng);
  }

  SampleType sample_corrupted(const SampleType& clean, std::mt19937* prng) {
    if (bb.sample(prng)) {
      return be.sample_corrupted(clean, prng);
    }
    return clean;
  }

  SampleType propose_clean(const std::vector<SampleType>& corrupted,
                           std::mt19937* prng) {
    // We approximate the maximum likelihood estimate by taking the mode of
    // corrupted.  The full solution would construct BaseEmissor and
    // BetaBernoulli instances for each choice of clean and picking the
    // clean with the highest combined logp_score().
    std::unordered_map<SampleType, int> counts;
    SampleType mode;
    int max_count = 0;
    for (const SampleType& c : corrupted) {
      ++counts[c];
      if (counts[c] > max_count) {
        max_count = counts[c];
        mode = c;
      }
    }
    return mode;
  }
};
