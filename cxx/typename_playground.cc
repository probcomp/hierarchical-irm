
#include "clean_relation.hh"
#include "domain.hh"
#include "util_distribution_variant.hh"

#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <typeinfo>

using RelationVariant = std::variant<Relation<std::string>*, Relation<double>*,
                                     Relation<int>*, Relation<bool>*>;

template <class T>
std::string type_name() {
  typedef typename std::remove_reference<T>::type TR;
  std::unique_ptr<char, void (*)(void*)> own(
#ifndef _MSC_VER
      abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
#else
      nullptr,
#endif
      std::free);
  std::string r = own != nullptr ? own.get() : typeid(TR).name();
  if (std::is_const<TR>::value) r += " const";
  if (std::is_volatile<TR>::value) r += " volatile";
  if (std::is_lvalue_reference<T>::value)
    r += "&";
  else if (std::is_rvalue_reference<T>::value)
    r += "&&";
  return r;
}

int main() {
  std::mt19937 prng;
  DistributionSpec dist_spec = DistributionSpec("bernoulli");
  DistributionVariant dv = get_prior(dist_spec, &prng);

  std::visit(
      [&](const auto& v) {
        std::cout << "decltype(*v) = " << type_name<decltype(*v)>() << '\n';
        std::cout << "std::remove_reference_t<decltype(*v)> = "
                  << type_name<std::remove_reference_t<decltype(*v)>>() << '\n';
        std::cout
            << "typename std::remove_reference_t<decltype(*v)>::SampleType = "
            << type_name<
                   typename std::remove_reference_t<decltype(*v)>::SampleType>()
            << '\n';
      },
      dv);

  std::vector<Domain*> domains;
  domains.push_back(new Domain("D1"));
  RelationVariant rv = new CleanRelation<bool>("r1", dist_spec, domains);

  std::visit(
      [&](const auto& relv) {
        std::cout << "decltype(*relv) = " << type_name<decltype(*relv)>()
                  << '\n';
      },
      rv);
}
