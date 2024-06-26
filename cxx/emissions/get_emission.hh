// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>

#include "emissions/sometimes.hh"

class BitFlip;
class GaussianEmission;
class SimpleStringEmission;
using SometimesBitFlip = Sometimes<BitFlip>;
using SometimesGaussian = Sometimes<GaussianEmission>;

using EmissionVariant = std::variant<GaussianEmission*, SometimesGaussian*,
                                     SometimesBitFlip*, SimpleStringEmission*>;

EmissionVariant get_emission(const std::string& emission_name);
