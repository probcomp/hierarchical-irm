// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <vector>

#include "distributions/base.hh"
#include "distributions/dirichlet_categorical.hh"

// A distribution over a finite set of strings.
class StringCat : public Distribution<std::string> {
 public:
  std::vector<std::string> strings;
  DirichletCategorical dc;

  // Each element of vs should be distinct.
  StringCat(const std::vector<std::string> &vs) : strings(vs), dc(vs.size()) {};

  int string_to_index(const std::string& s) const;

  void incorporate(const std::string& s, double weight = 1.0);

  double logp(const std::string& s) const;

  double logp_score() const;

  std::string sample(std::mt19937* prng);

  void set_alpha(double alphat);

  void transition_hyperparameters(std::mt19937* prng);

  std::string nearest(const std::string& x) const;
};
