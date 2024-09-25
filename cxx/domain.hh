// Copyright 2020
// See LICENSE.txt

#pragma once
#include <cassert>
#include <string>
#include <unordered_set>

#include "distributions/crp.hh"

typedef int T_item;

class Domain {
 public:
  const std::string name;            // human-readable name
  std::unordered_set<T_item> items;  // set of items
  CRP crp;                           // clustering model for items

  Domain(const std::string& name) : name(name), crp() { assert(!name.empty()); }
  void incorporate(std::mt19937* prng, const T_item& item, int table = -1) {
    if (items.contains(item)) {
      assert(table == -1);
    } else {
      if (table == -1) {
        assert(prng != nullptr);
      }
      items.insert(item);
      int t = 0 <= table ? table : crp.sample(prng);
      crp.incorporate(item, t);
    }
  }
  void unincorporate(const T_item& item) {
    assert(items.count(item) == 1);
    crp.unincorporate(item);
    items.erase(item);
  }
  int get_cluster_assignment(const T_item& item) const {
    assert(items.contains(item));
    return crp.assignments.at(item);
  }
  void set_cluster_assignment_gibbs(const T_item& item, int table) {
    assert(items.contains(item));
    assert(crp.assignments.at(item) != table);
    crp.unincorporate(item);
    crp.incorporate(item, table);
  }
  std::unordered_map<int, double> tables_weights() const {
    return crp.tables_weights();
  }
  std::unordered_map<int, double> tables_weights_gibbs(
      const T_item& item) const {
    int table = get_cluster_assignment(item);
    return crp.tables_weights_gibbs(table);
  }
};
