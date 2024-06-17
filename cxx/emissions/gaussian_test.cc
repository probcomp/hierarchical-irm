// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Gaussian

#include "emissions/gaussian.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  GaussianEmission ge;

  BOOST_TEST(ge.logp_score() == 0., tt::tolerance(1e-6));
  ge.incorporate(std::make_pair(1.0, 1.1));
  BOOST_TEST(ge.N == 1);
  ge.incorporate(std::make_pair(5.0, 4.9));
  BOOST_TEST(ge.N == 2);
  ge.unincorporate(std::make_pair(1.0, 1.1));
  BOOST_TEST(ge.N == 1);

  BOOST_TEST(ge.logp_score() == -1.0472020831064763, tt::tolerance(1e-6));

  BOOST_TEST(ge.logp(std::make_pair(2.0, 2.001)) == -0.80065106134957509,
             tt::tolerance(1e-6));

  std::mt19937 prng;
  double dirty = ge.sample_corrupted(5.0, &prng);
  BOOST_TEST(dirty < 6.0);
  BOOST_TEST(dirty > 4.0);

  double clean = ge.propose_clean({-5.0, -4.9, -5.1, -5.2, -4.8}, &prng);
  BOOST_TEST(clean == -5.0);
}
