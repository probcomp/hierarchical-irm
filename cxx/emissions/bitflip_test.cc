// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test BitFlip

#include "emissions/bitflip.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

BOOST_AUTO_TEST_CASE(test_simple) {
  BitFlip bf;

  BOOST_TEST(bf.logp_score() == 0.0);
  BOOST_TEST(bf.N == 0);
  bf.incorporate(std::make_pair<bool, bool>(true, false));
  BOOST_TEST(bf.logp_score() == 0.0);
  BOOST_TEST(bf.N == 1);
  bf.unincorporate(std::make_pair<bool, bool>(true, false));
  BOOST_TEST(bf.logp_score() == 0.0);
  BOOST_TEST(bf.N == 0);
  bf.incorporate(std::make_pair<bool, bool>(false, true));
  bf.incorporate(std::make_pair<bool, bool>(false, true));
  BOOST_TEST(bf.logp_score() == 0.0);
  BOOST_TEST(bf.N == 2);

  BOOST_TEST(bf.logp(std::make_pair<bool, bool>(true, false)) == 0.0);

  std::mt19937 prng;
  BOOST_TEST(bf.sample_corrupted(false, &prng));

  BOOST_TEST(bf.propose_clean({false, false, false}, &prng));
}
