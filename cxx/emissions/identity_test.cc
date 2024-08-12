// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Identity

#include "emissions/identity.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

BOOST_AUTO_TEST_CASE(test_simple) {
  IdentityEmission<bool> ie;

  BOOST_TEST(ie.logp_score() == 0.0);
  BOOST_TEST(ie.N == 0);
  ie.incorporate(std::make_pair<bool, bool>(true, true));
  BOOST_TEST(ie.logp_score() == 0.0);
  BOOST_TEST(ie.N == 1);
  ie.unincorporate(std::make_pair<bool, bool>(true, true));
  BOOST_TEST(ie.logp_score() == 0.0);
  BOOST_TEST(ie.N == 0);

  ie.incorporate(std::make_pair<bool, bool>(false, false));
  ie.incorporate(std::make_pair<bool, bool>(false, false));
  BOOST_TEST(ie.logp_score() == 0.0);
  BOOST_TEST(ie.N == 2);

  BOOST_TEST(ie.logp(std::make_pair<bool, bool>(true, true)) == 0.0);

  std::mt19937 prng;
  BOOST_TEST(ie.propose_clean({true, true}, &prng));
}
