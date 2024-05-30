// Copyright 2024
// See LICENSE.txt

#pragma once
#include "base.hh"
#include "dirichlet_categorical.hh"

class Bigram : public Distribution<std::string> {
 private:
  void assert_valid_char(const char c) const { assert(c >= ' ' && c <= '~'); }
  size_t char_to_index(const char c) const {
    assert_valid_char(c);
    return c - ' ';
  }
  char index_to_char(const size_t i) const {
    const char c = i + ' ';
    assert_valid_char(c);
    return c;
  }
  std::vector<size_t> string_to_indices(const std::string& str) const {
    // Convert the string to a vector of indices between 0 and `num_chars`,
    // with a start/stop symbol at the beginning/end.
    std::vector<size_t> inds = {num_chars};
    for (const char& c : str) {
      inds.push_back(char_to_index(c));
    }
    inds.push_back(num_chars);
    return inds;
  }

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
  void incorporate(const std::string& x) {
    const std::vector<size_t> indices = string_to_indices(x);
    for (size_t i = 0; i != indices.size() - 1; ++i) {
      transition_dists[indices[i]].incorporate(indices[i + 1]);
    }
  }
  void unincorporate(const std::string& s) {
    const std::vector<size_t> indices = string_to_indices(s);
    for (size_t i = 0; i != indices.size() - 1; ++i) {
      transition_dists[indices[i]].unincorporate(indices[i + 1]);
    }
  }
  double logp(const std::string& s) const {
    const std::vector<size_t> indices = string_to_indices(s);
    double total_logp = 0.0;
    for (size_t i = 0; i != indices.size() - 1; ++i) {
      total_logp += transition_dists[indices[i]].logp(indices[i + 1]);
      // Incorporate each value so that subsequent probabilities are
      // conditioned on it.
      transition_dists[indices[i]].incorporate(indices[i + 1]);
    }
    for (size_t i = 0; i != indices.size() - 1; ++i) {
      transition_dists[indices[i]].unincorporate(indices[i + 1]);
    }
    return total_logp;
  }
  double logp_score() const {
    return std::transform_reduce(
        transition_dists.cbegin(), transition_dists.cend(), 0, std::plus{},
        [&](auto d) -> double { return d.logp_score(); });
  }
  std::string sample() {
    std::string sampled_string;
    // TODO(emilyaf): Reconsider the reserved length and maybe enforce a
    // max length.
    sampled_string.reserve(30);
    // Sample the first character conditioned on the stop/start symbol.
    size_t current_ind = num_chars;
    size_t next_ind = transition_dists[current_ind].sample();
    transition_dists[current_ind].incorporate(next_ind);
    current_ind = next_ind;

    // Sample additional characters until the stop/start symbol is sampled.
    // Incorporate the sampled character at each loop iteration so that
    // subsequent samples are conditioned on its observation.
    while (current_ind != num_chars) {
      sampled_string += index_to_char(current_ind);
      next_ind = transition_dists[current_ind].sample();
      transition_dists[current_ind].incorporate(next_ind);
      current_ind = next_ind;
    }
    unincorporate(sampled_string);
    return sampled_string;
  }

  void set_alpha(double alphat) {
    alpha = alphat;
    for (auto& trans_dist : transition_dists) {
      trans_dist.alpha = alpha;
    }
  }
};
