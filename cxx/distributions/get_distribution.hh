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

#include "util_observation.hh"
#include "distributions/base.hh"

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
  std::map<std::string, std::string> distribution_args;

  DistributionSpec(const std::string& dist_str,
                   const std::map<std::string, std::string>& _distribution_args = {});
  DistributionSpec() = default;
};

// If you are adding a new Distribution type to DistributionVariant, you will
// also need to update ObservationVariant and ObservationEnum in
// util_observation.h.
using DistributionVariant =
    std::variant<Distribution<bool>*,
                 Distribution<int>*,
                 Distribution<double>*,
                 Distribution<std::string>*>;

// `get_prior` is an overloaded function with one version that returns
// DistributionVariant and one that returns EmissionVariant, for ease of use in
// CleanRelation.
DistributionVariant get_prior(const DistributionSpec& spec, std::mt19937* prng);
