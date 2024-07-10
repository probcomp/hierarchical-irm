// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>

#include "emissions/base.hh"

using EmissionVariant = std::variant<Emission<double>*,
                                     Emission<bool>*,
                                     Emission<std::string>*,
                                     Emission<int>*>

EmissionVariant get_emission(
    const std::string& emission_name,
    std::map<std::string, std::string> distribution_args = {});
