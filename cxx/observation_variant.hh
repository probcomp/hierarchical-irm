#pragma once

#include <string>
#include <variant>

// Set of all (non-Emission) Distribution sample types.
using ObservationVariant = std::variant<double, int, bool, std::string>;
