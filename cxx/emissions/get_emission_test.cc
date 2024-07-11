#define BOOST_TEST_MODULE test get_emission

#include "emissions/get_emission.hh"

#include <boost/test/included/unit_test.hpp>

#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"

BOOST_AUTO_TEST_CASE(test_get_emission_gaussian) {
  EmissionVariant ev = get_emission("gaussian");
  Emission<double>* ge = std::get<Emission<double>*>(ev);

  ge->incorporate(std::make_pair<double, double>(2.0, 2.1));
  BOOST_TEST(ge->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_simple_string) {
  EmissionVariant ev = get_emission("simple_string");
  Emission<std::string>* sse = std::get<Emission<std::string>*>(ev);

  sse->incorporate(std::make_pair<std::string, std::string>("hello", "hi"));
  BOOST_TEST(sse->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_sometimes_gaussian) {
  EmissionVariant ev = get_emission("sometimes_gaussian");
  Emission<double>* sg = std::get<Emission<double>*>(ev);

  sg->incorporate(std::make_pair<double, double>(2.0, 2.1));
  BOOST_TEST(sg->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_sometimes_bitflip) {
  EmissionVariant ev = get_emission("sometimes_bitflip");
  Emission<bool>* sbf = std::get<Emission<bool>*>(ev);

  sbf->incorporate(std::make_pair<bool, bool>(true, true));
  BOOST_TEST(sbf->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_sometimes_categorical) {
  EmissionVariant ev = get_emission("sometimes_categorical(k=5)");
  Emission<int>* sc = std::get<Emission<int>*>(ev);

  sc->incorporate(std::make_pair<int, int>(0, 1));
  BOOST_TEST(sc->N == 1);
}
