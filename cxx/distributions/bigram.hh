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
  double alpha = 1;       // hyperparameter for all transition distributions.
  size_t max_length = 0;  // 0 means no maximum length
  char min_char;          // Character with smallest ASCII value.
  char max_char;          // Character with largest ASCII value.
  size_t num_chars;
  mutable std::vector<DirichletCategorical> transition_dists;

  Bigram(size_t _max_length = 80, char _min_char = ' ', char _max_char = '~')
      : max_length(_max_length), min_char(_min_char), max_char(_max_char) {
    num_chars = max_char - min_char + 1;
    const size_t total_chars = num_chars + 1;  // Include a start/stop symbol.

    // The distribution at index `i` represents `p(X_{j+1} | X_j == char_i)`.
    transition_dists.reserve(total_chars);
    for (size_t i = 0; i != total_chars; ++i) {
      transition_dists.emplace_back(total_chars);
    }
  }

  void incorporate(const std::string& x, double weight = 1.0);

  double logp(const std::string& s) const;

  double logp_score() const;

  std::string sample(std::mt19937* prng);

  void set_alpha(double alphat);

  void transition_hyperparameters(std::mt19937* prng);
};
