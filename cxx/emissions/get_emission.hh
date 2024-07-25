// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
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
  std::map<std::string, std::string> emission_args;

  EmissionSpec(const std::string& emission_str,
               const std::map<std::string, std::string>& _emission_args = {});
  EmissionSpec() = default;
};

// `get_prior` is an overloaded function with one version that returns
// DistributionVariant and one that returns EmissionVariant, for ease of use in
// CleanRelation.
EmissionVariant get_prior(const EmissionSpec& spec, std::mt19937* prng = nullptr);
