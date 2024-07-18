#pragma once

#include <cstdio>
#include <unordered_map>
#include <vector>

#include "string_alignment.hh"
#include "distributions/dirichlet_categorical.hh"
#include "emissions/base.hh"

// A string emission model that conditions the probability of substitutions,
// insertions and deletions on the current character.

class BigramStringEmission : public Emission<std::string> {
 public:
  char lowest_char = ' ';
  char highest_char = '~';
  // insertions.sample() == 0 means no insertion.
  std::vector<DirichletCategorical> insertions;
  // substitutions.sample() == 0 means deletion.
  std::vector<DirichletCategorical> substitutions;

  BigramStringEmission();

  void incorporate(const std::pair<std::string, std::string>& x,
                   double weight = 1.0);

  double logp(const std::pair<std::string, std::string>& x) const;

  double logp_score() const;

  void transition_hyperparameters(std::mt19937* prng);

  std::string sample_corrupted(const std::string& clean, std::mt19937* prng);

  std::string propose_clean(const std::vector<std::string>& corrupted,
                            std::mt19937* unused_prng);

  // The following methods are conceptually private, but actually public
  // for testing purposes.
  size_t get_index(char current_char);
  size_t get_index(std::string current_char);
  std::string category_to_char(int category);
  std::string two_string_vote(const std::string &s1, const std::string &s2);
  double log_prob_distance(const StrAlignment& alignment, double old_cost);
};
