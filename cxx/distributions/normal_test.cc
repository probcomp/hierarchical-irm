// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include "distributions/normal.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  std::mt19937 prng;
  Normal nd(&prng);

  nd.incorporate(5.0);
  nd.incorporate(-2.0);
  nd.unincorporate(5.0);
  nd.incorporate(7.0);
  nd.unincorporate(-2.0);

  BOOST_TEST(nd.logp(6.0) == -2.7673076255063034, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == -4.7299819282937534, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(no_nan_after_incorporate_unincorporate) {
  std::mt19937 prng;
  Normal nd(&prng);

  nd.incorporate(10.0);
  nd.unincorporate(10.0);

  BOOST_TEST(!std::isnan(nd.mean));
  BOOST_TEST(!std::isnan(nd.var));
}

BOOST_AUTO_TEST_CASE(logp_before_incorporate) {
  std::mt19937 prng;
  Normal nd(&prng);

  BOOST_TEST(nd.logp(6.0) == -4.4357424552958129, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0.0, tt::tolerance(1e-6));

  nd.incorporate(5.0);
  nd.unincorporate(5.0);

  BOOST_TEST(nd.logp(6.0) == -4.4357424552958129, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0.0, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(sample) {
  std::mt19937 prng;
  Normal nd(&prng);

  for (int i = 0; i < 1000; ++i) {
    nd.incorporate(42.0);
  }

  double s = nd.sample();

  BOOST_TEST(s > 40.0);
  BOOST_TEST(s < 44.0);
}

BOOST_AUTO_TEST_CASE(incorporate_raises_logp) {
  std::mt19937 prng;
  Normal nd(&prng);

  double old_lp = nd.logp(10.0);
  for (int i = 0; i < 10; ++i) {
    nd.incorporate(10.0);
    double lp = nd.logp(10.0);
    BOOST_TEST(lp > old_lp);
    old_lp = lp;
  }
}

BOOST_AUTO_TEST_CASE(prior_prefers_origin) {
  std::mt19937 prng;
  Normal nd1(&prng), nd2(&prng);

  for (int i = 0; i < 100; ++i) {
    nd1.incorporate(0.0);
    nd2.incorporate(50.0);
  }

  BOOST_TEST(nd1.logp_score() > nd2.logp_score());
}
