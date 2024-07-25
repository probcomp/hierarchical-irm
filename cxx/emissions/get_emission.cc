// Copyright 2024
// See LICENSE.txt

#include "emissions/get_emission.hh"

#include <cassert>
#include <random>
#include <string>

#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"

EmissionSpec::EmissionSpec(
    const std::string& emission_str,
    const std::map<std::string, std::string>& _emission_args):
  emission_args(_emission_args) {
  if (emission_str == "gaussian") {
    emission = EmissionEnum::gaussian;
    observation_type = ObservationEnum::double_type;
  } else if (emission_str == "simple_string") {
    emission = EmissionEnum::simple_string;
    observation_type = ObservationEnum::string_type;
  } else if (emission_str == "sometimes_gaussian") {
    emission = EmissionEnum::sometimes_gaussian;
    observation_type = ObservationEnum::double_type;
  } else if (emission_str == "sometimes_bitflip") {
    emission = EmissionEnum::sometimes_bitflip;
    observation_type = ObservationEnum::bool_type;
  } else {
    assert(false && "Unsupported emission name.");
  }
}

EmissionVariant get_prior(const EmissionSpec& spec, std::mt19937* prng) {
  switch (spec.emission) {
    case EmissionEnum::gaussian:
      return new GaussianEmission();
    case EmissionEnum::simple_string:
      return new SimpleStringEmission();
    case EmissionEnum::sometimes_bitflip:
      return new SometimesBitFlip();
    case EmissionEnum::sometimes_gaussian:
      return new SometimesGaussian();
    default:
      assert(false && "Unsupported emission enum value.");
  }
}
