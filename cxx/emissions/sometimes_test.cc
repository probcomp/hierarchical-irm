// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Sometimes

#include "emissions/sometimes.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

#include "emissions/bitflip.hh"

BOOST_AUTO_TEST_CASE(test_simple) {
  Sometimes<bool> sbf(new BitFlip());

  double orig_lp = sbf.logp_score();
  BOOST_TEST(sbf.N == 0);
  sbf.incorporate(std::make_pair<bool, bool>(true, false));
  BOOST_TEST(sbf.logp_score() < 0.0);
  BOOST_TEST(exp(sbf.logp(std::make_pair<bool, bool>(true, false))) + exp(sbf.logp(std::make_pair<bool, bool>(true, true))) == 1.0);
  BOOST_TEST(sbf.N == 1);
  sbf.unincorporate(std::make_pair<bool, bool>(true, false));
  BOOST_TEST(sbf.logp_score() == orig_lp);
  BOOST_TEST(sbf.N == 0);

  sbf.incorporate(std::make_pair<bool, bool>(false, true));
  sbf.incorporate(std::make_pair<bool, bool>(false, true));
  BOOST_TEST(sbf.logp_score() < 0.0);
  BOOST_TEST(sbf.N == 2);

  BOOST_TEST(sbf.logp(std::make_pair<bool, bool>(true, false)) < 0.0);

  std::mt19937 prng;
  BOOST_TEST(sbf.propose_clean({true, true, false}, &prng));
}
