#pragma once

#include <cassert>

#include "emissions/base.hh"

// A *deterministic* Emission class that always emits not(clean).
// Most users will want to combine this with Sometimes.
class BitFlip : public Emission<bool> {
 public:
  BitFlip() {};

  void incorporate(const std::pair<bool, bool>& x, double weight = 1.0) {
    assert(x.first != x.second);
    N += weight;
  }

  double logp(const std::pair<bool, bool>& x) const {
    assert(x.first != x.second);
    return 0.0;
  }

  double logp_score() const { return 0.0; }

  // No hyperparameters to transition!
  void transition_hyperparameters(std::mt19937* prng) {}

  bool sample_corrupted(const bool& clean, std::mt19937* unused_prng) {
    return !clean;
  }

  bool propose_clean(const std::vector<bool>& corrupted,
                     std::mt19937* unused_prng) {
    return !corrupted[0];
  }
};
