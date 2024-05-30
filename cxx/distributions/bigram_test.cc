// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Bigram

#include "distributions/bigram.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple) {
  std::mt19937 prng;
  Bigram bb(&prng);

  bb.incorporate("hello");
  bb.incorporate("world");
  bb.unincorporate("hello");
  bb.incorporate("train");
  bb.unincorporate("world");

  BOOST_TEST(bb.logp("test") == -22.169938638053061, tt::tolerance(1e-6));
  BOOST_TEST(bb.logp_score() == -24, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_set_alpha) {
  std::mt19937 prng;
  Bigram bb(&prng);

  bb.incorporate("hello");
  double first_lp = bb.logp_score();

  bb.set_alpha(2.0);
  for (auto trans_dist : bb.transition_dists) {
    BOOST_TEST(trans_dist.alpha == 2.0);
  }

  BOOST_TEST(first_lp != bb.logp_score());
}
