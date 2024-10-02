// Copyright 2024
// See LICENSE.txt

#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <limits>
#include "distributions/stringcat.hh"
#include "emissions/string_alignment.hh"

int StringCat::string_to_index(const std::string& s) const {
  auto it = std::find(strings.begin(), strings.end(), s);
  if (it == strings.end()) {
    printf("String %s not in StringCat's list of strings\n", s.c_str());
    std::exit(1);
  }
  return it - strings.begin();
}

void StringCat::incorporate(const std::string& s, double weight) {
  dc.incorporate(string_to_index(s), weight);
  N += weight;
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

std::string StringCat::nearest(const std::string& x) const {
  if (std::find(strings.begin(), strings.end(), x) != strings.end()) {
    return x;
  }

  const std::string *nearest = &(strings[0]);
  double lowest_distance = std::numeric_limits<double>::max();
  for (const std::string& s : strings) {
    std::vector<StrAlignment> alignments;
    topk_alignments(1, s, x, edit_distance, &alignments);
    double d = alignments[0].cost;
    if (d < lowest_distance) {
      lowest_distance = d;
      nearest = &s;
    }
  }

  return *nearest;
}
