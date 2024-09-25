// Copyright 2024
// See LICENSE.txt

#include "distributions/bigram.hh"

#include <cassert>
#include <cstdlib>

#include "distributions/base.hh"

void Bigram::assert_valid_char(const char c) const {
  assert(c >= min_char && c <= max_char);
}

size_t Bigram::char_to_index(const char c) const {
  assert_valid_char(c);
  return c - min_char;
}

char Bigram::index_to_char(const size_t i) const {
  const char c = i + min_char;
  assert_valid_char(c);
  return c;
}

std::vector<size_t> Bigram::string_to_indices(const std::string& str) const {
  // Convert the string to a vector of indices between 0 and `num_chars`,
  // with a start/stop symbol at the beginning/end.
  std::vector<size_t> inds = {num_chars};
  for (const char& c : str) {
    inds.push_back(char_to_index(c));
  }
  inds.push_back(num_chars);
  return inds;
}

void Bigram::incorporate(const std::string& x, double weight) {
  if ((max_length > 0) && (x.length() > max_length)) {
    printf("String %s has length %ld, but max length is %ld.\n", x.c_str(),
           x.length(), max_length);
    std::exit(1);
  }
  const std::vector<size_t> indices = string_to_indices(x);
  for (size_t i = 0; i != indices.size() - 1; ++i) {
    transition_dists[indices[i]].incorporate(indices[i + 1], weight);
  }
  N += weight;
}

double Bigram::logp(const std::string& s) const {
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

double Bigram::logp_score() const {
  double logp = 0;
  for (const auto& d : transition_dists) {
    logp += d.logp_score();
  }
  return logp;
}

std::string Bigram::sample(std::mt19937* prng) {
  std::string sampled_string;
  if (max_length > 0) {
    sampled_string.reserve(max_length);
  } else {
    sampled_string.reserve(2 * num_chars);
  }
  // Sample the first character conditioned on the stop/start symbol.
  size_t current_ind = num_chars;
  size_t next_ind = transition_dists[current_ind].sample(prng);
  transition_dists[current_ind].incorporate(next_ind);
  current_ind = next_ind;

  // Sample additional characters until the stop/start symbol is sampled.
  // Incorporate the sampled character at each loop iteration so that
  // subsequent samples are conditioned on its observation.
  while (current_ind != num_chars) {
    sampled_string += index_to_char(current_ind);
    if (sampled_string.length() == max_length) {
      break;
    }
    next_ind = transition_dists[current_ind].sample(prng);
    transition_dists[current_ind].incorporate(next_ind);
    current_ind = next_ind;
  }
  N += 1;  // Correct for calling unincorporate below.
  unincorporate(sampled_string);
  return sampled_string;
}

void Bigram::set_alpha(double alphat) {
  alpha = alphat;
  for (auto& trans_dist : transition_dists) {
    trans_dist.alpha = alpha;
  }
}

void Bigram::transition_hyperparameters(std::mt19937* prng) {
  std::vector<double> logps;
  std::vector<double> alphas;
  // C++ doesn't yet allow range for-loops over existing variables.  Sigh.
  for (double alphat : ALPHA_GRID) {
    set_alpha(alphat);
    double lp = logp_score();
    if (!std::isnan(lp)) {
      logps.push_back(logp_score());
      alphas.push_back(alphat);
    }
  }
  if (logps.empty()) {
    printf("Warning!  All hyperparameters for Bigram give nans!\n");
    assert(false);
  } else {
    int i = sample_from_logps(logps, prng);
    set_alpha(alphas[i]);
  }
}
