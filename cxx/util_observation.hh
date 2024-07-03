// Copyright 2024
// See LICENSE.txt

// Classes and functions for dealing with Distributions and their values in a
// generic manner.  When a new subclass is added, this file needs to be updated.

#pragma once

#include <string>
#include <variant>

// Set of all observation types.
using ObservationVariant = std::variant<double, int, bool, std::string>;
enum class ObservationEnum { double_type, int_type, bool_type, string_type };

ObservationVariant observation_string_to_value(
    const std::string& value_str, const ObservationEnum& observation_type);
