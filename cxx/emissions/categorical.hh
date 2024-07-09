#pragma once

#include <cassert>
#include <limits>
#include <utility>

#include "distributions/dirichlet_categorical.hh"
#include "emissions/base.hh"

// A "bigram" emission model that tracks separate emission distributions per
// clean categorical state.
class CategoricalEmission : public Emission<int> {
 public:
  mutable std::vector<DirichletCategorical> emission_dists;

  CategoricalEmission(int num_states) {
    emission_dists.reserve(num_states);
    for (int i = 0; i < num_states; ++i) {
      emission_dists.emplace_back(num_states);
    }
  };

  void incorporate(const std::pair<int, int>& x, double weight = 1.0) {
    N += weight;
    emission_dists[x.first].incorporate(x.second, weight);
  }

  double logp(const std::pair<int, int>& x) const {
    return emission_dists[x.first].logp(x.second);
  }

  double logp_score() const {
    double lp = 0.0;
    for (const auto& e : emission_dists) {
      lp += e.logp_score();
    }
    return lp;
  }

  void transition_hyperparameters(std::mt19937* prng) {
    for (auto& e : emission_dists) {
      e.transition_hyperparameters(prng);
    }
  }

  int sample_corrupted(const int& clean, std::mt19937* prng) {
    return emission_dists[clean].sample(prng);
  }

  int propose_clean(const std::vector<int>& corrupted,
                     std::mt19937* unused_prng) {
    // Brute force; compute log prob over all possible clean states.
    int best_clean;
    double best_clean_logp = std::numeric_limits<double>::lowest();
    for (size_t i = 0; i < emission_dists.size(); ++i) {
      double lp = 0.0;
      for (const auto& c : corrupted) {
        lp += emission_dists[i].logp(c);
      }
      if (lp > best_clean_logp) {
        best_clean = i;
        best_clean_logp = lp;
      }
    }
    return best_clean;
  }

};
