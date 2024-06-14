#pragma once

#include <cstdio>
#include <unordered_map>

#include "distributions/beta_bernoulli.hh"
#include "emissions/base.hh"

// A simple string emission model that handles substitutions, insertions
// and deletions of characters, all without context.  (I.e., their
// probability doesn't depend on any previous character or the character
// being substituted, inserted or deleted.)

class SimpleStringEmission : public Emission<std::string> {
 public:
  char lowest_char = ' ';
  char highest_char = '~';
  BetaBernoulli substitutions;
  BetaBernoulli insertions;
  BetaBernoulli deletions;

  SimpleStringEmission(){};

  void incorporate(const std::pair<std::string, std::string>& x) {
    ++N;
    corporate(x.first, x.second, true);
  }

  void unincorporate(const std::pair<std::string, std::string>& x) {
    --N;
    corporate(x.first, x.second, false);
  }

  // Calculate the insertions, deletion and substitutions between clean and
  // dirty through a greedy, recursive search.  Incorporate those diffs into
  // the corresponding BetaBernoulli models if d is true; otherwise
  // unincorporate them.
  void corporate(const std::string& clean, const std::string& dirty, bool d) {
    if (clean.empty() && dirty.empty()) {
      // If both are empty, this is evidence of the lack of an insertion!
      d ? insertions.incorporate(0) : insertions.unincorporate(0);
      return;
    }

    if (clean.empty()) {
      // All of dirty must be insertions.
      for (size_t i = 0; i < dirty.length(); ++i) {
        d ? insertions.incorporate(1) : insertions.unincorporate(1);
      }
      return;
    }

    if (dirty.empty()) {
      // All of clean must have be deleted.
      for (size_t i = 0; i < clean.length(); ++i) {
        d ? deletions.incorporate(1) : deletions.unincorporate(1);
        // This is also evidence that clean.length()+1 insertions didn't happen.
        d ? insertions.incorporate(0) : insertions.unincorporate(0);
      }
      return;
    }

    if (clean[0] == dirty[0]) {
      // A match means there was no insertion, substitution or deletion.
      if (d) {
        substitutions.incorporate(0);
        insertions.incorporate(0);
        deletions.incorporate(0);
      } else {
        substitutions.unincorporate(0);
        insertions.unincorporate(0);
        deletions.unincorporate(0);
      }
      // Recurse on the rest of clean and dirty.
      corporate(clean.substr(1, std::string::npos),
                dirty.substr(1, std::string::npos), d);
      return;
    }

    if (clean.back() == dirty.back()) {
      // Check if clean[-1] == dirty[-1].  This block might be removeable.
      if (d) {
        substitutions.incorporate(0);
        insertions.incorporate(0);
        deletions.incorporate(0);
      } else {
        substitutions.unincorporate(0);
        insertions.unincorporate(0);
        deletions.unincorporate(0);
      }
      corporate(clean.substr(0, clean.length() - 1),
                dirty.substr(0, dirty.length() - 1), d);
      return;
    }

    // Here, the "right" thing to do would be to run a std::string alignment
    // algorithm.  But that would require a cost model, which we don't have.
    // Also, dealing with multiple alignments (which again, is the right thing
    // to do) would require passing fractional counts to our BetaBernoulli
    // distributions, which we can't do right now.
    // So instead, we just guess based on the std::string lengths.  Fun fact:
    // this will never overcount insertions or deletions!  It will merely
    // overcount substitutions by possibly putting the insertion or deletions
    // in less than optimal places.
    if (clean.length() < dirty.length()) {
      // Probably an insertion.
      d ? insertions.incorporate(1) : insertions.unincorporate(1);
      corporate(clean, dirty.substr(1, std::string::npos), d);
      return;
    }

    if (clean.length() > dirty.length()) {
      // Probably a deletion.
      d ? deletions.incorporate(1) : deletions.unincorporate(1);
      // Which means there (probably) wasn't an insertion.
      d ? insertions.incorporate(0) : insertions.unincorporate(0);
      corporate(clean.substr(1, std::string::npos), dirty, d);
      return;
    }

    // Probably a substitution.
    d ? substitutions.incorporate(1) : substitutions.unincorporate(1);
    // Which is evidence of no insertion or deletion.
    d ? insertions.incorporate(0) : insertions.unincorporate(0);
    d ? deletions.incorporate(0) : deletions.unincorporate(0);
    corporate(clean.substr(1, std::string::npos),
              dirty.substr(1, std::string::npos), d);
  }

  double logp(const std::pair<std::string, std::string>& x) const {
    const_cast<SimpleStringEmission*>(this)->incorporate(x);
    double lp = logp_score();
    const_cast<SimpleStringEmission*>(this)->unincorporate(x);
    return lp - logp_score();
  }

  double logp_score() const {
    return substitutions.logp_score() + insertions.logp_score() +
           deletions.logp_score()
           // Substitutions and insertions require us to generate a random char.
           + (substitutions.s + insertions.s) *
                 log(highest_char + 1 - lowest_char);
  }

  void transition_hyperparameters(std::mt19937* prng) {
    substitutions.transition_hyperparameters(prng);
    insertions.transition_hyperparameters(prng);
    deletions.transition_hyperparameters(prng);
  }

  char random_character(std::mt19937* prng) {
    std::uniform_int_distribution<char> uid(lowest_char, highest_char + 1);
    return uid(*prng);
  }

  std::string sample_corrupted(const std::string& clean, std::mt19937* prng) {
    std::string s;
    for (size_t i = 0; i < clean.length(); ++i) {
      while (insertions.sample(prng)) {
        s += random_character(prng);
      }
      if (deletions.sample(prng)) {
        continue;
      }
      if (substitutions.sample(prng)) {
        s += random_character(prng);
      } else {
        s += clean[i];
      }
    }
    while (insertions.sample(prng)) {
      s += random_character(prng);
    }
    return s;
  }

  std::string propose_clean(const std::vector<std::string>& corrupted,
                            std::mt19937* unused_prng) {
    std::string clean;
    // This implemention does simple voting per absolute string position.
    // A better version would first average the corrupted string lengths to
    // get a target length for clean, and then for each position i in
    // clean, find the mode among the
    // corrupted[j][i * corrupted[j].length / clean_length]
    size_t i = 0;
    while (true) {
      std::unordered_map<char, int> counts;
      int max_count = 0;
      char mode;
      for (const std::string& s : corrupted) {
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
      clean += mode;
      ++i;
    }
  }
};
