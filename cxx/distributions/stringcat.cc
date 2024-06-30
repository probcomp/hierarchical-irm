// Copyright 2024
// See LICENSE.txt

#include <algorithm>
#include <cassert>
#include "distributions/stringcat.hh"

int StringCat::string_to_index(const std::string& s) const {
  auto it = std::find(strings.begin(), strings.end(), s);
  if (it == strings.end()) {
    assert(false);
  }
  return it - strings.begin();
}

void StringCat::incorporate(const std::string& s) {
  dc.incorporate(string_to_index(s));
  ++N;
}

void StringCat::unincorporate(const std::string& s) {
  dc.unincorporate(string_to_index(s));
  --N;
}

double StringCat::logp(const std::string& s) const {
  return dc.logp(string_to_index(s));
}

double StringCat::logp_score() const {
  return dc.logp_score();
}

std::string StringCat::sample(std::mt19937* prng) {
  return strings[dc.sample(prng)];
}

void StringCat::transition_hyperparameters(std::mt19937* prng) {
  dc.transition_hyperparameters(prng);
}
