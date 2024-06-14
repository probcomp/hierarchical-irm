// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Bigram

#include "distributions/bigram.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple) {
  Bigram bg;

  bg.incorporate("hello");
  bg.incorporate("world");
  BOOST_TEST(bg.N == 2);
  bg.unincorporate("hello");
  BOOST_TEST(bg.N == 1);
  bg.incorporate("train");
  bg.unincorporate("world");
  BOOST_TEST(bg.N == 1);

  BOOST_TEST(bg.logp("test") == -22.169938638053061, tt::tolerance(1e-6));
  BOOST_TEST(bg.logp_score() == -27.386089148807059, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_set_alpha) {
  Bigram bg;

  bg.incorporate("hello");
  double first_lp = bg.logp_score();

  bg.set_alpha(2.0);
  for (auto trans_dist : bg.transition_dists) {
    BOOST_TEST(trans_dist.alpha == 2.0);
  }

  BOOST_TEST(first_lp != bg.logp_score(), tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(transition_hyperparameters) {
  std::mt19937 prng;
  Bigram bg;

  bg.transition_hyperparameters(&prng);
  for (int i = 0; i < 100; ++i) {
    bg.incorporate("abcdefghijklmnopqrstuvwxyz");
  }

  bg.transition_hyperparameters(&prng);

  BOOST_TEST(bg.alpha < 1.0);
}
