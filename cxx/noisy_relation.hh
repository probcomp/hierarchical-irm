// Copyright 2020
// See LICENSE.txt

#pragma once

#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "distributions/base.hh"
#include "domain.hh"
#include "emissions/base.hh"
#include "non_noisy_relation.hh"
#include "relation.hh"
#include "util_distribution_variant.hh"
#include "util_hash.hh"
#include "util_math.hh"

// T_noisy_relation is the text we get from reading a line of the schema file;
// NoisyRelation is the object that does the work.
class T_noisy_relation {
 public:
  // The relation is a map from the domains to the space .distribution
  // is a distribution over.
  std::vector<std::string> domains;

  // Name of the relation for the "true" values that the NoisyRelation observes.
  std::string base_relation;

  // Indicates if the NoisyRelation's values are observed or latent.
  bool is_observed;

  // Describes the Emission that models the noise.
  EmissionSpec emission_spec;
};

template <typename T>
class NoisyRelation : public Relation<T> {
 public:
  typedef T ValueType;

  // human-readable name
  const std::string name;
  // list of domain pointers
  const std::vector<Domain*> domains;
  // map from item to observed data
  std::unordered_map<const T_items, ValueType, H_items> data;
  // Base relation for "" values.
  const Relation<ValueType>* base_relation;
  // A Relation for the Emission that models noisy values given  values.
  NonNoisyRelation<std::pair<ValueType, ValueType>> emission_relation;

  NoisyRelation(const std::string& name, const EmissionSpec& emission_spec,
                const std::vector<Domain*>& domains, Relation<T>* base_relation)
      : name(name),
        domains(domains),
        base_relation(base_relation),
        emission_relation(name + "_emission", emission_spec, domains) {}

  void incorporate(std::mt19937* prng, const T_items& items, ValueType value) {
    data[items] = value;
    const ValueType _val = get_base_value(items);
    emission_relation.incorporate(prng, items, std::make_pair(_val, value));
  }

  void unincorporate(const T_items& items) {
    emission_relation.unincorporate(items);
  }

  double logp_gibbs_approx(const Domain& domain, const T_item& item, int table,
                           std::mt19937* prng) {
    return emission_relation.logp_gibbs_approx(domain, item, table, prng);
  }

  std::vector<double> logp_gibbs_exact(const Domain& domain, const T_item& item,
                                       std::vector<int> tables,
                                       std::mt19937* prng) {
    return emission_relation.logp_gibbs_exact(domain, item, tables, prng);
  }

  double logp(const T_items& items, ValueType value, std::mt19937* prng) {
    const ValueType _val = get_base_value(items);
    return emission_relation.logp(items, std::make_pair(_val, value), prng);
  }

  double logp_score() const { return emission_relation.logp_score(); }

  void set_cluster_assignment_gibbs(const Domain& domain, const T_item& item,
                                    int table, std::mt19937* prng) {
    emission_relation.set_cluster_assignment_gibbs(domain, item, table, prng);
  }

  bool has_observation(const Domain& domain, const T_item& item) const {
    return emission_relation.has_observation(domain, item);
  }

  const ValueType& get_value(const T_items& items) const {
    return std::get<0>(emission_relation.get_value(items));
  }

  const std::unordered_map<const T_items, ValueType, H_items>& get_data()
      const {
    return data;
  }

  const std::vector<Domain*>& get_domains() const { return domains; }

  const ValueType get_base_value(const T_items& items) const {
    size_t base_arity = base_relation->get_domains().size();
    T_items base_items(items.cbegin(), items.cbegin() + base_arity);
    return base_relation->get_value(base_items);
  }

  void transition_cluster_hparams(std::mt19937* prng, int num_theta_steps) {
    emission_relation.transition_cluster_hparams(prng, num_theta_steps);
  }

  std::vector<int> get_cluster_assignment(const T_items& items) const {
    return emission_relation.get_cluster_assignment(items);
  }

  double cluster_or_prior_logp(std::mt19937* prng, const T_items& z,
                               const ValueType& value) const {
    const ValueType base_value = get_base_value(z);
    if (emission_relation.clusters.contains(z)) {
      return emission_relation.clusters.at(z)->logp(
          std::make_pair(base_value, value));
    }
    auto emission_prior = emission_relation.make_new_distribution(prng);
    double emission_logp =
        emission_prior->logp(std::make_pair(base_value, value));
    delete emission_prior;
    return emission_logp;
  }

  // Disable copying.
  NoisyRelation& operator=(const NoisyRelation&) = delete;
  NoisyRelation(const NoisyRelation&) = delete;
};
