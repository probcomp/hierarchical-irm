// Copyright 2024
// Refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include <boost/test/included/unit_test.hpp>
#include "globals.hh"
#include "distributions/adapter.hh"

BOOST_AUTO_TEST_CASE(adapt_normal)
{
  PRNG prng;
  Normal nd(&prng);
  DistributionAdapter<double> ad(&nd);

  ad.incorporate("5.0");
  ad.incorporate("-2.0");
  ad.unincorporate("5.0");
  ad.incorporate("7.0");
  ad.unincorporate("-2.0");

  BOOST_TEST(nd.logp("6.0") == -3.1331256657870137, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == -4.7494000141508543, tt::tolerance(1e-6));

  string samp = ad.sample();
}
