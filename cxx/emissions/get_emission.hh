// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>

#include "emissions/sometimes.hh"
#include "util_observation.hh"

class BitFlip;
class GaussianEmission;
class SimpleStringEmission;
using SometimesBitFlip = Sometimes<BitFlip>;
using SometimesGaussian = Sometimes<GaussianEmission>;

enum class EmissionEnum {
  gaussian,
  simple_string,
  sometimes_bitflip,
  sometimes_gaussian
};
using EmissionVariant = std::variant<GaussianEmission*, SometimesGaussian*,
                                     SometimesBitFlip*, SimpleStringEmission*>;

struct EmissionSpec {
  EmissionEnum emission;
  ObservationEnum observation_type;

  EmissionSpec(const std::string& emission_str);
  EmissionSpec() = default;
};

EmissionVariant cluster_prior_from_spec(const EmissionSpec& spec,
                                        std::mt19937* prng = nullptr);