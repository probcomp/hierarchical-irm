#pragma once

#include <unordered_map>
#include "emissions/base.hh"
#include "distributions/beta_bernoulli.hh"

// A simple string emission model that handles substitutions, insertions
// and deletions of characters, all without context.  (I.e., their
// probability doesn't depend on any previous character or the character
// being substituted, inserted or deleted.)

class SimpleStringEmission : public Emission<std::string> {
 public:
  BetaBernoulli substitution;
  BetaBernoulli insertion;
  BetaBernoulli deletion;

  SimpleStringEmission() : substitution(nullptr),
                           insertion(nullptr),
                           deletion(nullptr) {};

  void incorporate(const std::pair<std::string, std::string>& x) {
    ++N;
    corporate(x.first, x.second, true);
  }

  void unincorporate(const std::pair<std::string, std::string>& x) {
    ++N;
    corporate(x.first, x.second, false);
  }

  void corporate(const std::string& clean, const std::string& dirty, bool d) {
    if (clean.empty()) {
      // All of dirty must be insertions.
      for (size_t i = 0; i < dirty.length(); ++i) {
        insertion.incorporate(d);
      }
      return;
    }

    if (dirty.empty()) {
      // All of clean must have be deleted.
      for (size_t i = 0; i < dirty.length(); ++i) {
        deletion.incorporate(d);
      }
      return;
    }

    if (clean[0] == dirty[0]) {
      substitution.incorporate(!d);
      insertion.incorporate(!d);
      deletion.incorporate(!d);
      corporate(clean.substr(1, std::string::npos),
                dirty.substr(1, std::string::npos),
                d);
      return;
    }

    if (clean.back() == dirty.back()) {
      substitution.incorporate(!d);
      insertion.incorporate(!d);
      deletion.incorporate(!d);
      corporate(clean.substr(0, clean.length() - 1),
                dirty.substr(0, dirty.length() - 1),
                d);
      return;
    }

    // Here, the "right" thing to do would be to run a std::string alignment
    // algorithm.  But that would require a cost model, which we don't have.
    // Also, dealing with multiple alignments (which again, is the right thing
    // to do) would require passing fractional counts to our BetaBernoulli
    // distributions, which we can't do right now.
    // So instead, we just guess based on the std::string lengths.
    if (clean.length() < dirty.length()) {
      // Probably an insertion.
      insertion.incorporate(d);
      corporate(clean, dirty.substr(1, std::string::npos), d);
      return;
    }

    if (clean.length() > dirty.length()) {
      // Probably a deletion.
      deletion.incorporate(d);
      corporate(clean.substr(1, std::string::npos), dirty, d);
      return;
    }

    // Probably a substitution.
    substitution.incorporate(d);
    corporate(clean.substr(1, std::string::npos),
              dirty.substr(1, std::string::npos),
              d);
  }

  double logp(const std::pair<std::string, std::string>& x) const {
    incorporate(x);
    double lp = logp_score();
    unincorporate(x);
    return lp - logp_score();
  }

  double logp_score() const {
    return substitution.logp_score() + insertion.logp_score() +
        deletion.logp_score();
  }

  void transition_hyperparameters() {
    substitution.transition_hyperparameters();
    insertion.transition_hyperparameters();
    deletion.transition_hyperparameters();
  }

  char random_character(std::mt19937* prng) {
    std::uniform_int_distribution<char> uid(' ', '~' + 1);
    return uid(*prng);
  }

  std::string sample_corrupted(const std::string& clean, std::mt19937* prng) {
    substitution.prng = prng;
    insertion.prng = prng;
    deletion.prng = prng;
    std::string s;
    for (size_t i = 0; i < clean.length(); ++i) {
      while (insertion.sample()) {
        s += random_character(prng);
      }
      if (deletion.sample()) {
        continue;
      }
      if (substitution.sample()) {
        s += random_character(prng);
      } else {
        s += clean[i];
      }
    }
    while (insertion.sample()) {
      s += random_character(prng);
    }
    return s;
  }

  std::string propose_clean(
      const std::vector<std::string>& corrupted,
      std::mt19937* unused_prng) {
    std::string clean;
    size_t i = 0;
    while (true) {
      std::unordered_map<char, int> counts;
      int max_count = 0;
      char mode;
      for (const std::string& s: corrupted) {
        char c;
        if (i < s.length()) {
          c = s[i];
        } else {
          c = '\0';
        }
        ++counts[c];
        if (counts[c] > max_count) {
          max_count = counts[c];
          mode = c;
        }
      }
      if (mode == '\0') {
        return clean;
      }
      clean = clean + mode;
    }
  }

}
