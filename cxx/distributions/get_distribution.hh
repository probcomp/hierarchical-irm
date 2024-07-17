// Copyright 2024
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <variant>
#include <boost/mp11.hpp>

#include "observation_variant.hh"
#include "distributions/base.hh"

// DistributionVariant is a std::variant of Distribution<T>* for every
// T in the list of sample_types defined in observation_variant.hh.  If you
// want to add a distribution with a new sample type, you will need to update
// that list.
template<class T> using apply_distribution = Distribution<T>*;
using distribution_of_sample_types = boost::mp11::mp_transform<apply_distribution, sample_types>;
using DistributionVariant = boost::mp11::mp_rename<distribution_of_sample_types, std::variant>;

// TODO(thomaswc): Define all of these get_distribution's using
// metaprogramming on sample_types.

// Return a distribution based on dist_string, which can either be a
// distribution name like "normal" or a name with parameters like
// "categorical(k=5)" or "stringcat(strings=a b c d)".
DistributionVariant get_distribution(
    const std::string& dist_string, std::mt19937* prng);

// Define get_distribution_from_distribution_variant, but only for types
// in the sample_types list.
template<typename T,
         typename std::enable_if<!boost::mp11::mp_contains<sample_types, T>::value, bool>::type = 0>
void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<T>** dist_out) {
  printf("Error!  get_distribution_from_distribution_variant called with non-DistributionVariant type!\n");
  assert(false);
  *dist_out = nullptr;
}

template<typename T,
         typename std::enable_if<boost::mp11::mp_contains<sample_types, T>::value, bool>::type = 0>
void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<T>** dist_out) {
  *dist_out = std::get<Distribution<T>*>(dv);
}
