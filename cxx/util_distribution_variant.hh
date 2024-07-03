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
#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"
#include "emissions/sometimes.hh"

enum class DistributionEnum {
  bernoulli,
  bigram,
  categorical,
  normal,
  skellam,
  stringcat
};

enum class EmissionEnum { sometimes_bitflip, gaussian, simple_string };

struct DistributionSpec {
  DistributionEnum distribution;
  std::map<std::string, std::string> distribution_args = {};
};

struct EmissionSpec {
  EmissionEnum emission;
};

// Set of all distribution sample types.
using ObservationVariant = std::variant<double, int, bool, std::string>;

using DistributionVariant =
    std::variant<BetaBernoulli*, Bigram*, DirichletCategorical*, Normal*,
                 Skellam*, StringCat*>;

using EmissionVariant =
    std::variant<Sometimes<BitFlip>*, GaussianEmission*, SimpleStringEmission*>;

ObservationVariant observation_string_to_value(
    const std::string& value_str, const DistributionEnum& distribution);

DistributionSpec parse_distribution_spec(const std::string& dist_str);

DistributionVariant cluster_prior_from_spec(const DistributionSpec& spec,
                                            std::mt19937* prng);

EmissionVariant cluster_prior_from_spec(const EmissionSpec& spec,
                                        std::mt19937* prng);
