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

  virtual void incorporate(std::mt19937* prng, const T_items& items,
                           ValueType value) = 0;

  // Unincorporates `items` and its corresponding value from the relation. Note
  // that the entities in `items` are not unincorporated from their respective
  // Domain CRPs, even if they are no longer contained in the relation's data,
  // since they may appear in other relations. Use `irm::unincorporate` to
  // ensure that entities not contained in any relation's data are
  // unincorporated from the domains as well.
  virtual void unincorporate(const T_items& items) = 0;

  virtual double logp(const T_items& items, ValueType value,
                      std::mt19937* prng) = 0;

  virtual double logp_score() const = 0;

  virtual double cluster_or_prior_logp(std::mt19937* prng,
                                       const std::vector<int>& z,
                                       const ValueType& value) const = 0;

  virtual double cluster_or_prior_logp_from_items(
      std::mt19937* prng, const T_items& items,
      const ValueType& value) const = 0;

  virtual ValueType sample_at_items(std::mt19937* prng,
                                    const T_items& items) const = 0;

  // Takes a sample from the cluster containing `items` and incorporates it.
  // Returns the sampled value.
  virtual ValueType sample_and_incorporate(std::mt19937* prng,
                                           const T_items& items) = 0;

  virtual void cleanup_data(const T_items& items) = 0;

  virtual void cleanup_clusters() = 0;

  virtual void incorporate_to_cluster(const T_items& items,
                                      const ValueType& value) = 0;

  virtual void unincorporate_from_cluster(const T_items& items) = 0;

  // TODO(emilyaf): Standardize passing PRNG first or last.
  virtual std::vector<double> logp_gibbs_exact(const Domain& domain,
                                               const T_item& item,
                                               std::vector<int> tables,
                                               std::mt19937* prng) = 0;

  virtual void set_cluster_assignment_gibbs(const Domain& domain,
                                            const T_item& item, int table,
                                            std::mt19937* prng) = 0;

  virtual void transition_cluster_hparams(std::mt19937* prng,
                                          int num_theta_steps) = 0;

  // Accessor/convenience methods, mostly for subclass members that can't be
  // accessed through the base class.
  virtual const std::vector<Domain*>& get_domains() const = 0;

  virtual const ValueType& get_value(const T_items& items) const = 0;

  virtual const std::unordered_map<const T_items, ValueType, H_items>&
  get_data() const = 0;

  virtual const std::unordered_map<
      std::string,
      std::unordered_map<T_item, std::unordered_set<T_items, H_items>>>&
  get_data_r() const = 0;

  virtual void update_value(const T_items& items, const ValueType& value) = 0;

  virtual std::vector<int> get_cluster_assignment(
      const T_items& items) const = 0;

  virtual bool has_observation(const Domain& domain,
                               const T_item& item) const = 0;

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
