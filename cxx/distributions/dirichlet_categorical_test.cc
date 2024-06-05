// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test DirichletCategorical

#include "distributions/dirichlet_categorical.hh"

#include <boost/test/included/unit_test.hpp>

#include "distributions/beta_bernoulli.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_matches_beta_bernoulli) {
  std::mt19937 prng;
  // 2-category Dirichlet Categorical is the same as a BetaBernoulli.
  DirichletCategorical dc(&prng, 2);
  BetaBernoulli bb(&prng);

  for (int i = 0; i < 10; ++i) {
    dc.incorporate(i % 2);
    bb.incorporate(i % 2);
  }
  for (int i = 0; i < 10; i += 2) {
    dc.unincorporate(i % 2);
    bb.unincorporate(i % 2);
  }

  BOOST_TEST(dc.N == 5);

  BOOST_TEST(dc.logp(1) == bb.logp(1), tt::tolerance(1e-6));
  BOOST_TEST(dc.logp(0) == bb.logp(0), tt::tolerance(1e-6));
  BOOST_TEST(dc.logp_score() == bb.logp_score(), tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_simple) {
  std::mt19937 prng;
  DirichletCategorical dc(&prng, 10);

  for (int i = 0; i < 10; ++i) {
    dc.incorporate(i);
  }
  for (int i = 0; i < 10; i += 2) {
    dc.unincorporate(i);
  }

  BOOST_TEST(dc.N == 5);
  BOOST_TEST(dc.logp(1) == -2.0149030205422647, tt::tolerance(1e-6));
  BOOST_TEST(dc.logp_score() == -12.389393702657209, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters) {
  std::mt19937 prng;
  DirichletCategorical dc(&prng, 10);

  dc.transition_hyperparameters();

  for (int i = 0; i < 100; ++i) {
    dc.incorporate(i % 10);
  }

  BOOST_TEST(dc.N == 100);
  dc.transition_hyperparameters();
  BOOST_TEST(dc.alpha > 1.0);
}
