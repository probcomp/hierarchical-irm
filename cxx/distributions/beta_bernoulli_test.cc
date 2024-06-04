// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test BetaBernoulli

#include "distributions/beta_bernoulli.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple) {
  std::mt19937 prng;
  BetaBernoulli bb(&prng);

  bb.incorporate(1);
  bb.incorporate(0);
  bb.unincorporate(1);
  bb.incorporate(1);
  bb.unincorporate(0);

  BOOST_TEST(bb.logp(1) == -0.4054651081081645, tt::tolerance(1e-6));
  BOOST_TEST(bb.logp_score() == -0.69314718055994529, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters)
{
  std::mt19937 prng;
  BetaBernoulli bb(&prng);

  bb.transition_hyperparameters();

  bb.incorporate(0);
  for (int i = 0; i < 100; ++i) {
    bb.incorporate(1);
  }

  bb.transition_hyperparameters();
  BOOST_TEST(bb.alpha > bb.beta);
}
