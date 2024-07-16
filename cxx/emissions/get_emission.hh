// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>
#include <boost/mp11.hpp>

#include "observation_variant.hh"
#include "emissions/base.hh"

// EmissionVariant is a std::variant of Emission<T>* for every
// T in the list of sample_types defined in observation_variant.hh.  If you
// want to add an emission with a new sample type, you will need to update
// that list.
template<class T> using apply_emission = Emission<T>*;
using emission_of_sample_types = boost::mp11::mp_transform<apply_emission, sample_types>;
using EmissionVariant = boost::mp11::mp_rename<emission_of_sample_types, std::variant>;

// Return an EmissionVariant based on emission_string, which can either be
// an emission name like "gaussian" or a name with parameters like
// "categorical(k=5)".
EmissionVariant get_emission(const std::string& emission_string,
                             std::mt19937* prng = nullptr);

template <typename T>
void get_distribution_from_emission_variant(
    const EmissionVariant &ev, Distribution<std::pair<T, T>>** dist_out) {
  *dist_out = std::get<Emission<T>*>(ev);
}

template <typename T>
void get_distribution_from_emission_variant(
    const EmissionVariant &ev, Distribution<T>** dist_out) {
  printf("Error!  Can't get Distribution<T> from an Emission unless T is of pair-type!\n");
  assert(false);
  *dist_out = nullptr;
}
