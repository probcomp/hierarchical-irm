// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include <boost/test/included/unit_test.hpp>
#include "distributions/normal.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple)
{
  PRNG prng;
  Normal nd(&prng);

  nd.incorporate(5.0);
  nd.incorporate(-2.0);
  nd.unincorporate(5.0);
  nd.incorporate(7.0);
  nd.unincorporate(-2.0);

  BOOST_TEST(nd.logp(6.0) == -3.1331256657870137, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == -4.7494000141508543, tt::tolerance(1e-6));
}

// TODO(thomaswc): Add more tests, including checking numbers versus
// github.com/probcomp/cgpm/src/primitives/normal.py.
