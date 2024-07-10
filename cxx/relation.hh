// Copyright 2020
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "domain.hh"
#include "util_distribution_variant.hh"
#include "util_hash.hh"

typedef std::vector<T_item> T_items;
typedef VectorIntHash H_items;

template <typename T>
class Relation {
 public:
  typedef T ValueType;

  virtual void incorporate(std::mt19937* prng, const T_items& items, ValueType value) = 0;

  virtual void unincorporate(const T_items& items) = 0;

  virtual double logp(const T_items& items, ValueType value, std::mt19937* prng) = 0;

  virtual double logp_score() const = 0;

  virtual double cluster_or_prior_logp(std::mt19937* prng, const T_items& items, const ValueType& value) const = 0;

  virtual std::vector<double> logp_gibbs_exact(
      const Domain& domain, const T_item& item, std::vector<int> tables,
      std::mt19937* prng) = 0;

  virtual void set_cluster_assignment_gibbs(const Domain& domain, const T_item& item,
                                            int table, std::mt19937* prng) = 0;

  virtual void transition_cluster_hparams(std::mt19937* prng, int num_theta_steps) = 0;

  // Accessor/convenience methods, mostly for subclass members that can't be accessed through the base class.
  virtual const std::vector<Domain*>& get_domains() const = 0;

  virtual const ValueType& get_value(const T_items& items) const = 0;

  virtual const std::unordered_map<const T_items, ValueType, H_items>& get_data() const = 0;

  virtual std::vector<int> get_cluster_assignment(const T_items& items) const = 0;

  virtual bool has_observation(const Domain& domain, const T_item& item) const = 0;

  virtual ~Relation() = default;

};
