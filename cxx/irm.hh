// Copyright 2020
// See LICENSE.txt

#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "clean_relation.hh"
#include "noisy_relation.hh"
#include "util_distribution_variant.hh"

using T_relation = std::variant<T_clean_relation, T_noisy_relation>;
using RelationVariant = std::variant<Relation<std::string>*, Relation<double>*,
                                     Relation<int>*, Relation<bool>*>;

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

  IRM(const T_schema& init_schema);

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
          observations,
      std::mt19937* prng);

  double logp_score() const;

  std::vector<Domain*> add_domains(const std::string& name,
                                   const std::vector<std::string>& domains);

  void add_relation(const std::string& name, const T_clean_relation& relation);

  void add_relation(const std::string& name, const T_noisy_relation& relation);

  void add_relation(const std::string& name, const T_noisy_relation& relation,
                    RelationVariant base_relation);

  void remove_relation(const std::string& name);

  // Disable copying.
  IRM& operator=(const IRM&) = delete;
  IRM(const IRM&) = delete;
};

// Run a single step of inference on an IRM model.
void single_step_irm_inference(std::mt19937* prng, IRM* irm, double& t_total,
                               bool verbose, int num_theta_steps = 10);
