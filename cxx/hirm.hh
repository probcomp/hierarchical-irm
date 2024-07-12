// Copyright 2020
// See LICENSE.txt

#pragma once
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "irm.hh"
#include "relation.hh"
#include "observation_variant.hh"

class HIRM {
 public:
  T_schema schema;                     // schema of relations
  std::unordered_map<int, IRM*> irms;  // map from cluster id to IRM
  std::unordered_map<std::string, int>
      relation_to_code;  // map from relation name to code
  std::unordered_map<int, std::string>
      code_to_relation;  // map from code to relation
  std::unordered_map<std::string, std::vector<std::string>>
      base_to_noisy_relation;  // map from relation to the noisy relation that
                               // has it as a base
  CRP crp;                     // clustering model for relations

  HIRM(const T_schema& schema, std::mt19937* prng);

  void incorporate(std::mt19937* prng, const std::string& r,
                   const T_items& items, const ObservationVariant& value);
  void unincorporate(const std::string& r, const T_items& items);

  int relation_to_table(const std::string& r);
  IRM* relation_to_irm(const std::string& r);
  RelationVariant get_relation(const std::string& r);
  void add_relation_to_irm(IRM* irm, const std::string& r,
                           const T_relation& t_relation);

  void transition_cluster_assignments_all(std::mt19937* prng);
  void transition_cluster_assignments(std::mt19937* prng,
                                      const std::vector<std::string>& rs);
  void transition_cluster_assignment_relation(std::mt19937* prng,
                                              const std::string& r);

  void set_cluster_assignment_gibbs(std::mt19937* prng, const std::string& r,
                                    int table);
  void update_base_relation(const std::string& r);

  void add_relation(std::mt19937* prng, const std::string& name,
                    const T_relation& rel);

  void remove_relation(const std::string& name);

  double logp(
      const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
          observations,
      std::mt19937* prng);

  double logp_score() const;

  ~HIRM();

  // Disable copying.
  HIRM& operator=(const HIRM&) = delete;
  HIRM(const HIRM&) = delete;
};
