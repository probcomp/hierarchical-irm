#pragma once

#include <string>
#include <variant>
#include <boost/mp11.hpp>

// List of all sample types.  If you want to use a Distribution<T> or an
// Emission<T>, then T should be on this list.
using sample_types = boost::mp11::mp_list<bool, double, int, std::string>;

// All of the sample types as a std::variant.
using ObservationVariant = boost::mp11::mp_rename<sample_types, std::variant>;
