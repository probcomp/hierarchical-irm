// Copyright 2020
// See LICENSE.txt

#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "relation.hh"
#include "util_distribution_variant.hh"

// Map from names to T_relation's.
typedef std::map<std::string, T_relation> T_schema;

class IRM {
 public:
  T_schema schema;                                   // schema of relations
  std::unordered_map<std::string, Domain*> domains;  // map from name to Domain
  std::unordered_map<std::string, RelationVariant>
      relations;  // map from name to Relation
  std::unordered_map<std::string, std::unordered_set<std::string>>
      domain_to_relations;  // reverse map

  IRM(const T_schema& schema);

  ~IRM();

  void incorporate(std::mt19937* prng, const std::string& r,
                   const T_items& items, ObservationVariant value);

  void unincorporate(const std::string& r, const T_items& items);

  void transition_cluster_assignments_all(std::mt19937* prng);

  void transition_cluster_assignments(std::mt19937* prng,
                                      const std::vector<std::string>& ds);

  void transition_cluster_assignment_item(std::mt19937* prng,
                                          const std::string& d,
                                          const T_item& item);
  double logp(
      const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
          observations);

  double logp_score() const;

  void add_relation(const std::string& name, const T_relation& relation);

  void remove_relation(const std::string& name);

  // Disable copying.
  IRM& operator=(const IRM&) = delete;
  IRM(const IRM&) = delete;
};


// Run a single step of inference on an IRM model.
void single_step_irm_inference(std::mt19937* prng, IRM* irm, double& t_total,
                               bool verbose);
