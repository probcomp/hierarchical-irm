// Copyright 2024
// See LICENSE.txt

// Classes and functions for dealing with Distributions and their values in a
// generic manner.  When a new subclass is added, this file needs to be updated.

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

class BetaBernoulli;
class Bigram;
class DirichletCategorical;
class Normal;

enum class DistributionEnum { bernoulli, bigram, categorical, normal };

struct DistributionSpec {
  DistributionEnum distribution;
  std::map<std::string, std::string> distribution_args = {};
};

// Set of all distribution sample types.
using ObservationVariant = std::variant<double, int, bool, std::string>;

using DistributionVariant =
    std::variant<BetaBernoulli*, Bigram*, DirichletCategorical*, Normal*>;

ObservationVariant observation_string_to_value(
    const std::string& value_str, const DistributionEnum& distribution);

DistributionSpec parse_distribution_spec(const std::string& dist_str);

DistributionVariant cluster_prior_from_spec(const DistributionSpec& spec);
