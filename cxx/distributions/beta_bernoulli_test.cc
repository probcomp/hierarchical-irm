// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test BetaBernoulli

#include <boost/test/included/unit_test.hpp>
#include "distributions/beta_bernoulli.hh"

BOOST_AUTO_TEST_CASE(test_simple)
{
  BetaBernoulli bb;
  bb.incorporate(1);
  bb.incorporate(0);
  bb.unincorporate(1);
  bb.incorporate(1);
  bb.unincorporate(0);

  BOOST_TEST(bb.logp(1) == 3.14);
  BOOST_TEST(bb.logp_score() == 3.14);
}
