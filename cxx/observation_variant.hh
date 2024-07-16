#pragma once

#include <string>
#include <variant>

template <class... Types> struct type_list {};

// List of all sample types.  If you want to use a Distribution<T> or an
// Emission<T>, then T should be on this list.
using sample_types = type_list<bool, double, int, std::string>;

// All of the sample types as a std::variant.
template<class... Types>
std::variant<Types...> as_variant(type_list<Types...>);

using ObservationVariant = decltype(as_variant(sample_types{}));
