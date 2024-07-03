#define BOOST_TEST_MODULE test get_emission

#include "emissions/get_emission.hh"

#include <boost/test/included/unit_test.hpp>

#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"

BOOST_AUTO_TEST_CASE(test_get_emission_gaussian) {
  EmissionVariant ev = cluster_prior_from_spec(EmissionSpec("gaussian"));
  GaussianEmission* ge = std::get<GaussianEmission*>(ev);

  ge->incorporate(std::make_pair<double, double>(2.0, 2.1));
  BOOST_TEST(ge->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_simple_string) {
  EmissionVariant ev = cluster_prior_from_spec(EmissionSpec("simple_string"));
  SimpleStringEmission* sse = std::get<SimpleStringEmission*>(ev);

  sse->incorporate(std::make_pair<std::string, std::string>("hello", "hi"));
  BOOST_TEST(sse->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_sometimes_gaussian) {
  EmissionVariant ev =
      cluster_prior_from_spec(EmissionSpec("sometimes_gaussian"));
  SometimesGaussian* sg = std::get<SometimesGaussian*>(ev);

  sg->incorporate(std::make_pair<double, double>(2.0, 2.1));
  BOOST_TEST(sg->N == 1);
}

BOOST_AUTO_TEST_CASE(test_get_emission_sometimes_bitflip) {
  EmissionVariant ev =
      cluster_prior_from_spec(EmissionSpec("sometimes_bitflip"));
  SometimesBitFlip* sbf = std::get<SometimesBitFlip*>(ev);

  sbf->incorporate(std::make_pair<bool, bool>(true, true));
  BOOST_TEST(sbf->N == 1);
}
