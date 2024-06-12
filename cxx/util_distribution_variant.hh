// Copyright 2024
// See LICENSE.txt

// This file collects classes/functions that depend on the set of distribution
// subclasses and should be updated when a new subclass is added.

#pragma once

#include <map>
#include <random>
#include <string>
#include <variant>
#include <vector>

enum class DistributionEnum { bernoulli, bigram, categorical, normal };

struct DistributionSpec {
  DistributionEnum distribution;
  std::map<std::string, std::string> distribution_args = {};
};

class BetaBernoulli;
class Bigram;
class DirichletCategorical;
class Normal;
class Domain;
template <typename DistributionType>
class Relation;

// Set of all distribution sample types.
using ObservationVariant = std::variant<double, int, std::string>;

using DistributionVariant =
    std::variant<BetaBernoulli*, Bigram*, DirichletCategorical*, Normal*>;
using RelationVariant =
    std::variant<Relation<BetaBernoulli>*, Relation<Bigram>*,
                 Relation<DirichletCategorical>*, Relation<Normal>*>;

ObservationVariant observation_string_to_value(
    const std::string& value_str, const DistributionEnum& distribution);

DistributionSpec parse_distribution_spec(const std::string& dist_str);

DistributionVariant cluster_prior_from_spec(const DistributionSpec& spec,
                                            std::mt19937* prng);

RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains,
                                   std::mt19937* prng);
