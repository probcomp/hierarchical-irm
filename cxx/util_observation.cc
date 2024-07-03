// Copyright 2024
// See LICENSE.txt

#include "util_observation.hh"

#include <cassert>

ObservationVariant observation_string_to_value(
    const std::string& value_str, const ObservationEnum& observation_type) {
  switch (observation_type) {
    case ObservationEnum::double_type:
      return std::stod(value_str);
    case ObservationEnum::bool_type:
      return static_cast<bool>(std::stoi(value_str));
    case ObservationEnum::int_type:
      return std::stoi(value_str);
    case ObservationEnum::string_type:
      return value_str;
    default:
      assert(false && "Unsupported observation_type enum value.");
  }
}
