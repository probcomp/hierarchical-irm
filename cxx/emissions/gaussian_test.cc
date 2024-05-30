// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Gaussian

#include "emissions/gaussian.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  GaussianEmission ge;

  ge.incorporate(std::make_pair(1.0, 1.1));
  ge.incorporate(std::make_pair(5.0, 4.9));
  ge.unincorporate(std::make_pair(1.0, 1.1));

  BOOST_TEST(ge.logp_score() == 0.0, tt::tolerance(1e-6));
}
