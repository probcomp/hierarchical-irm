// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test CategoricalEmission

#include "emissions/categorical.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple) {
  CategoricalEmission ce(5);

  BOOST_TEST(ce.logp_score() == 0.0);
  BOOST_TEST(ce.N == 0);
  ce.incorporate(std::make_pair<int, int>(0, 2));
  BOOST_TEST(ce.N == 1);
  BOOST_TEST(ce.logp_score() == -1.6094379124341001, tt::tolerance(1e-6));
  ce.unincorporate(std::make_pair<int, int>(0, 2));
  BOOST_TEST(ce.N == 0);
  ce.incorporate(std::make_pair<int, int>(3, 3));
  ce.incorporate(std::make_pair<int, int>(4, 4));
  BOOST_TEST(ce.N == 2);

  BOOST_TEST(ce.logp(std::make_pair<int, int>(2, 2)) == -4.8283137373023006,
             tt::tolerance(1e-6));

  std::mt19937 prng;
  int s = ce.sample_corrupted(1, &prng);
  BOOST_TEST(s < 5);
  BOOST_TEST(s >= 0);

  int clean = ce.propose_clean({1, 1, 3, 4}, &prng);
  BOOST_TEST(clean < 5);
  BOOST_TEST(clean >= 0);
}
