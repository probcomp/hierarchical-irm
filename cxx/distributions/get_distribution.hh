// Copyright 2024
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <variant>

#include "distributions/base.hh"

using DistributionVariant =
    std::variant<Distribution<bool>*,
                 Distribution<double>*,
                 Distribution<int>*,
                 Distribution<std::string>*>;

// Return a distribution based on dist_string, which can either be a
// distribution name like "normal" or a name with parameters like
// "categorical(k=5)" or "stringcat(strings=a b c d)".
DistributionVariant get_distribution(
    const std::string& dist_string, std::mt19937* prng);