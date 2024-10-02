// Copyright 2020
// See LICENSE.txt

#pragma once

#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "distributions/get_distribution.hh"
#include "domain.hh"
#include "util_hash.hh"

typedef std::vector<T_item> T_items;
typedef VectorIntHash H_items;

template <typename T>
class Relation {
 public:
  typedef T ValueType;

  // Incorporates `items` and `value` into the relation, possibly creating a new
  // cluster.
  virtual void incorporate(std::mt19937* prng, const T_items& items,
                           ValueType value) = 0;

  // Unincorporates `items` and its corresponding value from the relation. Note
  // that the entities in `items` are not unincorporated from their respective
  // Domain CRPs, even if they are no longer contained in the relation's data,
  // since they may appear in other relations. Use `irm::unincorporate` to
  // ensure that entities not contained in any relation's data are
  // unincorporated from the domains as well.
  virtual void unincorporate(const T_items& items) = 0;

  // Returns the logp of items and value.
  virtual double logp(const T_items& items, ValueType value,
                      std::mt19937* prng) = 0;

  // Returns the cumulative logp of incorporated data.
  virtual double logp_score() const = 0;

  // Returns the logp with respect to the cluster (distribution) indexed by z
  // (or the prior logp, if there is no cluster at z).
  virtual double cluster_or_prior_logp(std::mt19937* prng,
                                       const std::vector<int>& z,
                                       const ValueType& value) const = 0;

  // Returns the logp with respect to the cluster (distribution) to which items
  // belongs (or the prior logp, if there is no such cluster).
  virtual double cluster_or_prior_logp_from_items(
      std::mt19937* prng, const T_items& items,
      const ValueType& value) const = 0;

  // Samples from the cluster (distribution) that contains items.
  virtual ValueType sample_at_items(std::mt19937* prng,
                                    const T_items& items) const = 0;

  // Takes a sample from the cluster containing `items` and incorporates it.
  // Returns the sampled value.
  virtual ValueType sample_and_incorporate(std::mt19937* prng,
                                           const T_items& items) = 0;

  // Removes `items` from both `data` and the reverse map `data_r`.
  virtual void cleanup_data(const T_items& items) = 0;

  // Removes any clusters (and their distribution models) that have no data
  // incorporated.
  virtual void cleanup_clusters() = 0;

  // Incorporates items and value into an existing cluster.
  virtual void incorporate_to_cluster(const T_items& items,
                                      const ValueType& value) = 0;

  // Unincorporates items from its cluster. Does not remove empty clusters.
  virtual void unincorporate_from_cluster(const T_items& items) = 0;

  // TODO(emilyaf): Standardize passing PRNG first or last.
  // Returns exact Gibbs logp.
  virtual std::vector<double> logp_gibbs_exact(const Domain& domain,
                                               const T_item& item,
                                               std::vector<int> tables,
                                               std::mt19937* prng) = 0;

  // Updates the cluster assignment of item.
  virtual void set_cluster_assignment_gibbs(const Domain& domain,
                                            const T_item& item, int table,
                                            std::mt19937* prng) = 0;

  // Gibbs kernel for cluster hparams.
  virtual void transition_cluster_hparams(std::mt19937* prng,
                                          int num_theta_steps) = 0;

  // Accessor/convenience methods, mostly for subclass members that can't be
  // accessed through the base class.
  virtual const std::vector<Domain*>& get_domains() const = 0;

  // Returns the value incorporated at items.
  virtual const ValueType& get_value(const T_items& items) const = 0;

  // Returns the incorporated items and values.
  virtual const std::unordered_map<const T_items, ValueType, H_items>&
  get_data() const = 0;

  // Returns the reverse data map.
  virtual const std::unordered_map<
      std::string,
      std::unordered_map<T_item, std::unordered_set<T_items, H_items>>>&
  get_data_r() const = 0;

  // Updates the value incorporated at items.
  virtual void update_value(const T_items& items, const ValueType& value) = 0;

  // Returns the cluster assignment of items.
  virtual std::vector<int> get_cluster_assignment(
      const T_items& items) const = 0;

  // Checks if a domain in the relation contains an item.
  virtual bool has_observation(const Domain& domain,
                               const T_item& item) const = 0;

  // Checks if items belongs to an existing cluster.
  virtual bool clusters_contains(const T_items& items) const = 0;

  // Convert a string to ValueType.
  ValueType from_string(const std::string& s) {
    ValueType t;
    std::stringstream ss(s);
    ss >> t;
    return t;
  };

  virtual ~Relation() = default;
};

// Overload from_string for ValueType=string because >> truncates at the first
// space.
template <>
std::string Relation<std::string>::from_string(const std::string& s);
