// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test DirichletCategorical

#include <boost/test/included/unit_test.hpp>
#include "distributions/dirichlet_categorical.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple)
{
  std::mt19937 prng;
  DirichletCategorical dc(&prng, 10);

  for (int i = 0; i < 10; ++i) {
    dc.incorporate(i);
  }
  for (int i = 0; i < 10; i += 2) {
    dc.unincorporate(i);
  }

  BOOST_TEST(dc.logp(1) == -2.0149030205422647, tt::tolerance(1e-6));
  BOOST_TEST(dc.logp_score() == -12.389393702657209, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters)
{
  std::mt19937 prng;
  DirichletCategorical dc(&prng, 10);

  dc.transition_hyperparameters();

  for (int i = 0; i < 100; ++i) {
    dc.incorporate(i % 10);
  }

  dc.transition_hyperparameters();
  BOOST_TEST(dc.alpha > 1.0);
}

