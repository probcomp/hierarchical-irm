// Copyright 2024
// See LICENSE.txt

#pragma once

#include "distributions/base.hh"
#include "distributions/dirichlet_categorical.hh"

class Bigram : public Distribution<std::string> {
 private:
  void assert_valid_char(const char c) const;

  size_t char_to_index(const char c) const;

  char index_to_char(const size_t i) const;

  std::vector<size_t> string_to_indices(const std::string& str) const;

 public:
  double alpha = 1;  // hyperparameter for all transition distributions.
  size_t num_chars = '~' - ' ' + 1;  // printable ASCII without DEL.
  mutable std::vector<DirichletCategorical> transition_dists;
  std::mt19937* prng;

  // Bigram does not take ownership of prng.
  Bigram(std::mt19937* prng) {
    this->prng = prng;
    const size_t total_chars = num_chars + 1;  // Include a start/stop symbol.

    // The distribution at index `i` represents `p(X_{j+1} | X_j == char_i)`.
    transition_dists.reserve(total_chars);
    for (size_t i = 0; i != total_chars; ++i) {
      transition_dists.emplace_back(prng, total_chars);
    }
  }

  void incorporate(const std::string& x);

  void unincorporate(const std::string& s);

  double logp(const std::string& s) const;

  double logp_score() const;

  std::string sample();

  void set_alpha(double alphat);

  void transition_hyperparameters();
};
