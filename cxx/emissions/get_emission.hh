// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>

#include "emissions/base.hh"

using EmissionVariant = std::variant<Emission<bool>*,
                                     Emission<double>*,
                                     Emission<int>*,
                                     Emission<std::string>*>;

// Return an EmissionVariant based on emission_string, which can either be
// an emission name like "gaussian" or a name with parameters like
// "categorical(k=5)".
EmissionVariant get_emission(const std::string& emission_string,
                             std::mt19937* prng = nullptr);
