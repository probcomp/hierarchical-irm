// Copyright 2024
// See LICENSE.txt

#include "emissions/get_emission.hh"

#include <cassert>
#include <random>
#include <string>

#include "util_parse.hh"

#include "emissions/bitflip.hh"
#include "emissions/categorical.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"
#include "emissions/sometimes.hh"

EmissionVariant get_emission(
    const std::string& emission_string, std::mt19937* prng) {
  std::string emission_name;
  std::map<std::string, std::string> emission_args;
  parse_name_and_parameters(emission_string, &emission_name, &emission_args);

  if (emission_name == "gaussian") {
    return new GaussianEmission();
  } else if (emission_name == "simple_string") {
    return new SimpleStringEmission();
  } else if (emission_name == "sometimes_bitflip") {
    return new Sometimes<bool>(new BitFlip());
  } else if (emission_name == "sometimes_categorical") {
    int num_states = std::stoi(emission_args.at("k"));
    // return new Sometimes<int>(new CategoricalEmission(num_states), true);
    return new Sometimes<int>(new CategoricalEmission(num_states));
  } else if (emission_name == "sometimes_gaussian") {
    return new Sometimes<double>(new GaussianEmission());
  }
  printf("Unknown emission name %s\n", emission_name.c_str());
  assert(false);
}
