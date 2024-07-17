// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test bigram_string

#include "emissions/bigram_string.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_incorporate) {
  BigramStringEmission bse;
  double lp1 = bse.logp({"hi", "hi"});
  bse.incorporate({"bye", "bye"});
  BOOST_TEST(bse.N == 1);
  bse.unincorporate({"bye", "bye"});
  BOOST_TEST(bse.N == 0);
  double lp2 = bse.logp({"hi", "hi"});
  BOOST_TEST(lp1 == lp2);
}

BOOST_AUTO_TEST_CASE(test_sample_corrupted) {
  BigramStringEmission bse;
  std::mt19937 prng;
  // Incorporate some clean data.
  bse.incorporate({"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz"});
  std::string corrupted = bse.sample_corrupted("test", &prng);
  BOOST_TEST(corrupted.length() < 7);
}

BOOST_AUTO_TEST_CASE(test_propose_clean) {
  BigramStringEmission bse;
  std::mt19937 prng;
  BOOST_TEST(bse.propose_clean({"clean"}, &prng) == "clean");
  BOOST_TEST(
      bse.propose_clean({"clean", "clean!", "cl5an", "lean"}, &prng)
      == "clean");
}
