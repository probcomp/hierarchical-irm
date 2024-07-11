// Copyright 2024
// See LICENSE.txt

// Classes and functions for dealing with observations of different types in a
// generic manner.

#pragma once

#include <string>
#include <variant>

// Set of all observation types.
// ObservationVariant and ObservationEnum need not be listed in the same order.
using ObservationVariant = std::variant<double, int, bool, std::string>;
enum class ObservationEnum { double_type, int_type, bool_type, string_type };

ObservationVariant observation_string_to_value(
    const std::string& value_str, const ObservationEnum& observation_type);
