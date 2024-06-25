
#include "util_distribution_variant.hh"

#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>
#include <iostream>

template <class T>
std::string
type_name()
{
    typedef typename std::remove_reference<T>::type TR;
    std::unique_ptr<char, void(*)(void*)> own
           (
#ifndef _MSC_VER
                abi::__cxa_demangle(typeid(TR).name(), nullptr,
                                           nullptr, nullptr),
#else
                nullptr,
#endif
                std::free
           );
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}


int main() {
  DistributionSpec dist_spec = parse_distribution_spec("bernoulli");
  DistributionVariant dv = cluster_prior_from_spec(dist_spec);

  std::visit(
      [&](const auto& v) {
        std::cout << "decltype(*v) = " << type_name<decltype(*v)>() << '\n';
        std::cout << "std::remove_reference_t<decltype(*v)> = " <<
                  type_name<std::remove_reference_t<decltype(*v)>>() << '\n';
        std::cout << "typename std::remove_reference_t<decltype(*v)>::SampleType = " <<
                  type_name<typename std::remove_reference_t<decltype(*v)>::SampleType>() << '\n';
      }, dv);
}
