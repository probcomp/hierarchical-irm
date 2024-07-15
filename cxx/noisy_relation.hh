// Copyright 2020
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "clean_relation.hh"
#include "distributions/base.hh"
#include "domain.hh"
#include "emissions/base.hh"
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

  // Indicates if the NoisyRelation's values are observed or latent.
  bool is_observed;

  // Describes the Emission that models the noise.
  EmissionSpec emission_spec;

  // Name of the relation for the "true" values that the NoisyRelation observes.
  std::string base_relation;
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
  // Base relation for "true" values.
  Relation<ValueType>* base_relation;
  // map from items in the base relation to items in the NoisyRelation
  std::unordered_map<T_items, std::unordered_set<T_items, H_items>, H_items>
      base_to_noisy_items;
  // A Relation for the Emission that models noisy values given true values.
  CleanRelation<std::pair<ValueType, ValueType>> emission_relation;

  NoisyRelation(const std::string& name, const EmissionSpec& emission_spec,
                const std::vector<Domain*>& domains, Relation<T>* base_relation)
      : name(name),
        domains(domains),
        base_relation(base_relation),
        emission_relation(name + "_emission", emission_spec, domains) {}

  void incorporate(std::mt19937* prng, const T_items& items, ValueType value) {
    T_items base_items = get_base_items(items);
    const ValueType base_val = base_relation->get_value(base_items);
    emission_relation.incorporate(prng, items, std::make_pair(base_val, value));
    data[items] = value;
    base_to_noisy_items[base_items].insert(items);
  }

  // incorporate_to_cluster and unincorporate_from_cluster should be used with
  // care, since they mutate the clusters only and not the relation. In
  // particular, for every call to unincorporate_from_cluster, there must be a 
  // corresponding call to incorporate_to_cluster with the same items, or the
  // Relation will be in an invalid state. See the Attribute class for usage/
  // justification of this choice.
  void incorporate_to_cluster(const T_items& items, const ValueType& value) {
    const ValueType& base_val = get_base_value(items);
    emission_relation.incorporate_to_cluster(items,
                                             std::make_pair(base_val, value));
  }

  void unincorporate_from_cluster(const T_items& items) {
    emission_relation.unincorporate_from_cluster(items);
  }

  void unincorporate(const T_items& items) {
    assert(data.contains(items));
    emission_relation.unincorporate(items);
    data.erase(items);
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
    const ValueType base_val = get_base_value(items);
    return emission_relation.logp(items, std::make_pair(base_val, value), prng);
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
    assert(data.contains(items));
    return data.at(items);
  }

  const std::unordered_map<const T_items, ValueType, H_items>& get_data()
      const {
    return data;
  }

  void update_value(const T_items& items, const ValueType& value) {
    assert(data.contains(items));
    data.at(items) = value;
    const ValueType& base_value = get_base_value(items);
    emission_relation.update_value(items, std::make_pair(value, base_value));
  }

  const std::vector<Domain*>& get_domains() const { return domains; }

  const T_items get_base_items(const T_items& items) const {
    size_t base_arity = base_relation->get_domains().size();
    T_items base_items(items.cbegin(), items.cbegin() + base_arity);
    return base_items;
  }

  const ValueType get_base_value(const T_items& items) const {
    T_items base_items = get_base_items(items);
    return base_relation->get_value(base_items);
  }

  const std::unordered_map<const std::vector<int>, Emission<ValueType>*,
                           VectorIntHash>
  get_emission_clusters() const {
    std::unordered_map<const std::vector<int>, Emission<ValueType>*,
                       VectorIntHash>
        emission_clusters;
    for (const auto& [i, em] : emission_relation.clusters) {
      emission_clusters[i] = reinterpret_cast<Emission<ValueType>*>(em);
    }
    return emission_clusters;
  }

  void transition_cluster_hparams(std::mt19937* prng, int num_theta_steps) {
    emission_relation.transition_cluster_hparams(prng, num_theta_steps);
  }

  std::vector<int> get_cluster_assignment(const T_items& items) const {
    return emission_relation.get_cluster_assignment(items);
  }

  double cluster_or_prior_logp(std::mt19937* prng, const T_items& z,
                               const ValueType& value) const {
    if (emission_relation.clusters.contains(z)) {
      const ValueType base_value = get_base_value(z);
      return emission_relation.clusters.at(z)->logp(
          std::make_pair(base_value, value));
    }
    auto emission_prior = emission_relation.make_new_distribution(prng);
    // TODO(emilyaf): Need to plumb through a base value or e.g. sample it.
    double emission_logp = emission_prior->logp(std::make_pair(value, value));
    delete emission_prior;
    return emission_logp;
  }

  // Disable copying.
  NoisyRelation& operator=(const NoisyRelation&) = delete;
  NoisyRelation(const NoisyRelation&) = delete;
};
