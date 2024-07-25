// Copyright 2024
// See LICENSE.txt

// Classes and functions for dealing with observations of different types in a
// generic manner.

#pragma once

#include <string>
#include <variant>

// Set of all observation types.
// ObservationVariant and ObservationEnum need not be listed in the same order.
using ObservationVariant = std::variant<bool, double, int, std::string>;
enum class ObservationEnum { bool_type, double_type, int_type, string_type };
