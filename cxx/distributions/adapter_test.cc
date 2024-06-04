// Copyright 2024
// Refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include "distributions/adapter.hh"

#include <boost/test/included/unit_test.hpp>

#include "distributions/normal.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(adapt_normal) {
  std::mt19937 prng;
  DistributionAdapter<double> ad(new Normal(&prng));

  ad.incorporate("5.0");
  ad.incorporate("-2.0");
  ad.unincorporate("5.0");
  ad.incorporate("7.0");
  ad.unincorporate("-2.0");

  BOOST_TEST(ad.logp("6.0") == -2.7673076255063034, tt::tolerance(1e-6));
  BOOST_TEST(ad.logp_score() == -4.7299819282937534, tt::tolerance(1e-6));

  std::string samp = ad.sample();
}
