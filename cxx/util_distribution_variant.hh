// Copyright 2024
// See LICENSE.txt

// Classes and functions for dealing with Distributions and their values in a
// generic manner.  When a new subclass is added, this file needs to be updated.

#pragma once

#include <map>
#include <random>
#include <string>
#include <variant>
#include <vector>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "distributions/skellam.hh"
#include "distributions/stringcat.hh"
#include "util_observation.hh"

enum class DistributionEnum {
  bernoulli,
  bigram,
  categorical,
  normal,
  skellam,
  stringcat
};

struct DistributionSpec {
  DistributionEnum distribution;
  ObservationEnum observation_type;
  std::map<std::string, std::string> distribution_args = {};

  DistributionSpec(const std::string& dist_str,
                   std::map<std::string, std::string> distribution_args = {});
  DistributionSpec() = default;
};

using DistributionVariant =
    std::variant<BetaBernoulli*, Bigram*, DirichletCategorical*, Normal*,
                 Skellam*, StringCat*>;

ObservationVariant observation_string_to_value(
    const std::string& value_str, const ObservationEnum& observation_type);

// `get_prior` is an overloaded function with one version that returns
// DistributionVariant and one that returns EmissionVariant, for ease of use in
// CleanRelation.
DistributionVariant get_prior(const DistributionSpec& spec, std::mt19937* prng);
