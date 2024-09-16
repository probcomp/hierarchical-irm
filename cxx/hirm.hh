// Copyright 2020
// See LICENSE.txt

#pragma once
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "distributions/get_distribution.hh"
#include "irm.hh"
#include "observations.hh"
#include "relation.hh"
#include "transition_latent_value.hh"

class HIRM {
 public:
  T_schema schema;                     // schema of relations
  std::unordered_map<int, IRM*> irms;  // map from cluster id to IRM
  std::unordered_map<std::string, int>
      relation_to_code;  // map from relation name to code
  std::unordered_map<int, std::string>
      code_to_relation;  // map from code to relation
  std::unordered_map<std::string, std::vector<std::string>>
      base_to_noisy_relations;  // map from relation to the noisy relation that
                                // has it as a base
  CRP crp;                      // clustering model for relations

  HIRM(const T_schema& schema, std::mt19937* prng);

  void incorporate(std::mt19937* prng, const std::string& r,
                   const T_items& items, const ObservationVariant& value);
  void unincorporate(const std::string& r, const T_items& items);

  int relation_to_table(const std::string& r) const;
  IRM* relation_to_irm(const std::string& r) const;
  RelationVariant get_relation(const std::string& r) const;
  void add_relation_to_irm(IRM* irm, const std::string& r,
                           const T_relation& t_relation);

  void transition_cluster_assignments_all(std::mt19937* prng);
  void transition_cluster_assignments(std::mt19937* prng,
                                      const std::vector<std::string>& rs);
  void transition_cluster_assignment_relation(std::mt19937* prng,
                                              const std::string& r);

  // Updates the latent values contained in relation `r` using Gibbs sampling.
  // `r` must be the base relation of at least one noisy relation.
  void transition_latent_values_relation(std::mt19937* prng,
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

  // Samples `n` values from each leaf relation (i.e. each relation that is not
  // the base relation of a different relation). Recursively samples some number
  // of values from non-leaf relations as needed to get to `n` leaf samples.
  // Beware: since `n` unique values are sampled from CRPs, if `n` is too high
  // relative to the CRP `alpha`s, this function might take a very long time.
  // Returns the sampled relations.
  T_encoded_observations sample_and_incorporate(std::mt19937* prng, int n);

  // Incorporates a sample into relation `r`. If `r` is a noisy relation, this
  // function recursively incorporates a sample into the base relation, if
  // necessary.  Returns the sample value as a string.
  std::string sample_and_incorporate_relation(
      std::mt19937* prng, const std::string& r, T_items& items);

  // Return a map from domains to CRP's that are initialized with the entities
  // each domain has seen so far.
  void initialize_domain_crps(std::map<std::string, CRP>* domain_crps) const;

  ~HIRM();

  // Disable copying.
  HIRM& operator=(const HIRM&) = delete;
  HIRM(const HIRM&) = delete;
};

