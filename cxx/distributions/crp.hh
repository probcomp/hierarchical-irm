// Copyright 2024
// See LICENSE.txt

#pragma once
#include <random>
#include <unordered_map>
#include <unordered_set>

typedef int T_item;

// TODO(emilyaf): Make this a distribution subclass.
class CRP {
 public:
  double alpha = 1.;  // concentration parameter
  int N = 0;          // number of customers
  std::unordered_map<int, std::unordered_set<T_item>>
      tables;  // map from table id to set of customers
  std::unordered_map<T_item, int> assignments;  // map from customer to table id
  std::mt19937* prng;

  CRP(std::mt19937* prng) { this->prng = prng; }

  void incorporate(const T_item& item, int table);

  void unincorporate(const T_item& item);

  int sample();

  double logp(int table) const;

  double logp_score() const;

  std::unordered_map<int, double> tables_weights() const;

  std::unordered_map<int, double> tables_weights_gibbs(int table) const;

  void transition_alpha();
};
