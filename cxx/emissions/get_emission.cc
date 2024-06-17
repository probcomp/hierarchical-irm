// Copyright 2024
// See LICENSE.txt

#include "emissions/get_emission.hh"

#include <cassert>

#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"

EmissionVariant get_emission(const std::string& emission_name) {
  if (emission_name == "gaussian") {
    return new GaussianEmission();
  } else if (emission_name == "simple_string") {
    return new SimpleStringEmission();
  } else if (emission_name == "sometimes_gaussian") {
    return new SometimesGaussian();
  } else if (emission_name == "sometimes_bitflip") {
    return new SometimesBitFlip();
  }
  printf("Unknown emission name %s\n", emission_name.c_str());
  assert(false);
}
