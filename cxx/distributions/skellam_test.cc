// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Skellam

#include "distributions/skellam.hh"

#include <boost/test/included/unit_test.hpp>

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  Skellam sd;
  std::mt19937 prng;

  sd.init_theta(&prng);

  BOOST_TEST(sd.logp_score() == 0.0, tt::tolerance(1e-6));
  BOOST_TEST(sd.logp(6) == -8.2461659399497425, tt::tolerance(1e-6));

  sd.incorporate(5);
  sd.incorporate(2);
  BOOST_TEST(sd.N == 2);

  sd.unincorporate(5);
  sd.incorporate(7);
  BOOST_TEST(sd.N == 2);

  BOOST_TEST(sd.logp_score() == -12.676907210873877, tt::tolerance(1e-6));
  BOOST_TEST(sd.logp(6) == -8.2461659399497425, tt::tolerance(1e-6));

  int s = sd.sample(&prng);
  BOOST_TEST(s < 100.0);
}
