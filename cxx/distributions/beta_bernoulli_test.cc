// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test BetaBernoulli

#include "distributions/beta_bernoulli.hh"

#include <boost/math/distributions/bernoulli.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/quadrature/gauss_kronrod.hpp>
#include <boost/test/included/unit_test.hpp>

#include "util_math.hh"
namespace bm = boost::math;
namespace tt = boost::test_tools;
namespace bm = boost::math;

BOOST_AUTO_TEST_CASE(test_simple) {
  BetaBernoulli bb;

  bb.incorporate(1);
  bb.incorporate(0);
  BOOST_TEST(bb.N == 2);

  bb.unincorporate(1);
  BOOST_TEST(bb.N == 1);

  bb.incorporate(1);
  bb.unincorporate(0);
  BOOST_TEST(bb.N == 1);

  // We expect that this is log(Num ones + alpha) - log(N + alpha + beta) ==
  // log(2) - log(3) = log(2/3)
  BOOST_TEST(bb.logp(1) == log(2 / 3.), tt::tolerance(1e-6));
  // We expect a 50% chance when we see a single observation, since alpha = beta
  // = 1.
  BOOST_TEST(bb.logp_score() == -std::numbers::ln2, tt::tolerance(1e-6));

  bb.incorporate(1, 2.5);
  BOOST_TEST(bb.N == 3.5);
}

BOOST_AUTO_TEST_CASE(test_log_prob) {
  BetaBernoulli bb;

  bm::beta_distribution<> beta_dist(bb.alpha, bb.beta);

  for (int i = 0; i < 10; ++i) {
    bb.incorporate(static_cast<double>(i % 2));

    auto integrand = [&beta_dist, &i](double t) {
      bm::bernoulli_distribution bernoulli_dist(t);
      double result = bm::pdf(beta_dist, t);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(bernoulli_dist, static_cast<double>(j % 2));
      }
      return result;
    };
    double result = bm::quadrature::gauss_kronrod<double, 100>::integrate(
        integrand, 0., 1.);
    BOOST_TEST(bb.logp_score() == log(result), tt::tolerance(1e-5));
  }
}

BOOST_AUTO_TEST_CASE(test_posterior_predictive) {
  BetaBernoulli bb;

  bm::beta_distribution<> beta_dist(bb.alpha, bb.beta);

  double test_data = 1.;

  auto quad = bm::quadrature::gauss_kronrod<double, 100>();

  for (int i = 0; i < 11; ++i) {
    bb.incorporate(static_cast<double>(i % 2));

    auto pdata_integrand = [&beta_dist, &i](double t) {
      bm::bernoulli_distribution bernoulli_dist(t);
      double result = bm::pdf(beta_dist, t);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(bernoulli_dist, static_cast<double>(j % 2));
      }
      return result;
    };

    auto ppred_integrand = [&quad, &pdata_integrand, &test_data](double t) {
      bm::bernoulli_distribution bernoulli_dist(t);
      double result = pdata_integrand(t);
      double normalization = quad.integrate(pdata_integrand, 0., 1.);
      // Compute p(theta | data) = p(data | theta) p(theta) / p(data)
      result /= normalization;
      return bm::pdf(bernoulli_dist, test_data) * result;
    };

    // Compute int p(x | theta) p(theta | data)
    double result = quad.integrate(ppred_integrand, 0., 1.);
    BOOST_TEST(bb.logp(test_data) == log(result), tt::tolerance(1e-5));
  }
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters) {
  std::mt19937 prng;
  BetaBernoulli bb;

  bb.transition_hyperparameters(&prng);

  bb.incorporate(0);
  for (int i = 0; i < 100; ++i) {
    bb.incorporate(1);
  }

  BOOST_TEST(bb.N == 101);
  bb.transition_hyperparameters(&prng);
  // Expect that since we saw a lot of 1's, that alpha will be much larger than
  // beta.
  BOOST_TEST(bb.alpha > bb.beta);
}
