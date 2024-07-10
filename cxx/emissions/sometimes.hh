#pragma once

#include <cmath>
#include <unordered_map>

#include "distributions/beta_bernoulli.hh"
#include "emissions/base.hh"

// An Emission class that sometimes applies another Emission and sometimes
// doesn't.
template <typename SampleType = double>
class Sometimes : public Emission<SampleType> {
 public:
  BetaBernoulli bb;
  Emission<SampleType>* be;  // We own be.
  bool can_dirty_equal_clean;

  // By default, Sometimes only works with BaseEmissors that assign zero
  // probability to dirty == clean.  Pass _can_dirty_equal_clean=true to
  // override that assumption.
  Sometimes(Emission<SampleType>* _be, bool _can_dirty_equal_clean = false):
    be(_be), can_dirty_equal_clean(_can_dirty_equal_clean) {};

  ~Sometimes() {
    delete be;
  }

  void incorporate(const std::pair<SampleType, SampleType>& x,
                   double weight = 1.0) {
    this->N += weight;
    if (x.first != x.second) {
      bb.incorporate(true, weight);
      be->incorporate(x, weight);
      return;
    }
    if (can_dirty_equal_clean) {
      double p_came_from_be = exp(bb.logp(true) + be->logp(x));
      bb.incorporate(true, p_came_from_be * weight);
      be->incorporate(x, p_came_from_be * weight);
      bb.incorporate(false, (1.0 - p_came_from_be) * weight);
      return;
    }
    bb.incorporate(false, weight);
  }

  double logp(const std::pair<SampleType, SampleType>& x) const {
    if (x.first != x.second) {
      return bb.logp(true) + be->logp(x);
    }
    double lp = bb.logp(false);
    if (can_dirty_equal_clean) {
      lp += bb.logp(true) + be->logp(x);
    }
    return lp;
  }

  double logp_score() const { return bb.logp_score() + be->logp_score(); }

  void transition_hyperparameters(std::mt19937* prng) {
    be->transition_hyperparameters(prng);
    bb.transition_hyperparameters(prng);
  }

  SampleType sample_corrupted(const SampleType& clean, std::mt19937* prng) {
    if (bb.sample(prng)) {
      return be->sample_corrupted(clean, prng);
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
