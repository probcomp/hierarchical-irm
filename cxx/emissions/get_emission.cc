// Copyright 2024
// See LICENSE.txt

#include "emissions/get_emission.hh"

#include <cassert>
#include <cstdlib>
#include <random>
#include <string>

#include "emissions/bigram_string.hh"
#include "emissions/bitflip.hh"
#include "emissions/categorical.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"
#include "emissions/sometimes.hh"

EmissionSpec::EmissionSpec(
    const std::string& emission_str,
    const std::map<std::string, std::string>& _emission_args):
  emission_args(_emission_args) {
  if (emission_str == "bigram") {
    emission = EmissionEnum::bigram_string;
    observation_type = ObservationEnum::string_type;
  } else if (emission_str == "gaussian") {
    emission = EmissionEnum::gaussian;
    observation_type = ObservationEnum::double_type;
  } else if (emission_str == "simple_string") {
    emission = EmissionEnum::simple_string;
    observation_type = ObservationEnum::string_type;
  } else if (emission_str == "sometimes_bitflip") {
    emission = EmissionEnum::sometimes_bitflip;
    observation_type = ObservationEnum::bool_type;
  } else if (emission_str == "sometimes_categorical") {
    emission = EmissionEnum::sometimes_categorical;
    observation_type = ObservationEnum::int_type;
  } else if (emission_str == "sometimes_gaussian") {
    emission = EmissionEnum::sometimes_gaussian;
    observation_type = ObservationEnum::double_type;
  } else {
    printf("Unknown Emission name %s\n", emission_str.c_str());
    std::exit(1);
  }
}

EmissionVariant get_prior(const EmissionSpec& spec, std::mt19937* prng) {
  switch (spec.emission) {
    case EmissionEnum::bigram_string:
      return new BigramStringEmission();
    case EmissionEnum::gaussian:
      return new GaussianEmission();
    case EmissionEnum::simple_string:
      return new SimpleStringEmission();
    case EmissionEnum::sometimes_bitflip:
      return new Sometimes<bool>(new BitFlip());
    case EmissionEnum::sometimes_categorical:
      {
        int num_states = std::stoi(spec.emission_args.at("k"));
        return new Sometimes<int>(new CategoricalEmission(num_states), true);
      }
    case EmissionEnum::sometimes_gaussian:
      return new Sometimes<double>(new GaussianEmission());
    default:
      printf("Unknown Emission enum value %d.\n", (int)(spec.emission));
      std::exit(1);
  }
}
