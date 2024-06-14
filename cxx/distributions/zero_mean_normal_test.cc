// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include "distributions/zero_mean_normal.hh"

#include <boost/math/distributions/inverse_gamma.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/quadrature/gauss_kronrod.hpp>
#include <boost/test/included/unit_test.hpp>

#include "util_math.hh"
namespace bm = boost::math;
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  nd.incorporate(5.0);
  nd.incorporate(-2.0);
  BOOST_TEST(nd.N == 2);

  nd.unincorporate(5.0);
  nd.incorporate(7.0);
  BOOST_TEST(nd.N == 2);

  nd.unincorporate(-2.0);
  BOOST_TEST(nd.N == 1);
}

BOOST_AUTO_TEST_CASE(test_expected_vars) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);
  std::vector<double> entries{-1., 2., 4., 5., 6., 20.};
  std::vector<double> expected_vars{1.0, 2.5, 7.0, 11.5, 16.4, 241.0 / 3.0};
  for (int i = 0; i < std::ssize(entries); ++i) {
    nd.incorporate(entries[i]);
    BOOST_TEST(nd.var == expected_vars[i], tt::tolerance(1e-6));
  }

  for (int i = std::ssize(entries) - 1; i > 0; --i) {
    nd.unincorporate(entries[i]);
    BOOST_TEST(nd.var == expected_vars[i - 1], tt::tolerance(1e-6));
  }
}

BOOST_AUTO_TEST_CASE(no_nan_after_incorporate_unincorporate) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  nd.incorporate(10.0);
  nd.unincorporate(10.0);

  BOOST_TEST(nd.N == 0);
  BOOST_TEST(!std::isnan(nd.var));
}

BOOST_AUTO_TEST_CASE(test_log_prob) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  bm::inverse_gamma_distribution inv_gamma_dist(nd.alpha, nd.beta);
  auto quad = bm::quadrature::gauss_kronrod<double, 200>();

  for (int i = 0; i < 10; ++i) {
    nd.incorporate(i);

    auto integrand = [&nd, &inv_gamma_dist, &i](double ig) {
      bm::normal_distribution normal_dist(0., sqrt(ig));
      double result = bm::pdf(inv_gamma_dist, ig);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(normal_dist, j);
      }
      return result;
    };

    double result =
        quad.integrate(integrand, 0., std::numeric_limits<double>::infinity());

    BOOST_TEST(nd.logp_score() == log(result), tt::tolerance(1e-4));
  }
  BOOST_TEST(nd.N == 10);
}

BOOST_AUTO_TEST_CASE(test_posterior_pred) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  bm::inverse_gamma_distribution inv_gamma_dist(nd.alpha, nd.beta);
  auto quad = bm::quadrature::gauss_kronrod<double, 200>();

  for (int i = 0; i < 10; ++i) {
    double test_data = i / 3.;
    nd.incorporate(i);

    auto unnormalized = [&nd, &inv_gamma_dist, &i, &test_data](double ig) {
      bm::normal_distribution normal_dist(0., sqrt(ig));
      double result =
          bm::pdf(normal_dist, test_data) * bm::pdf(inv_gamma_dist, ig);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(normal_dist, j);
      }
      return result;
    };

    auto ppred_integrand = [&unnormalized, &nd](double ig) {
      double result = unnormalized(ig);
      double normalization = exp(nd.logp_score());
      // Compute p(theta | data) = p(data | theta) p(theta) / p(data)
      result /= normalization;
      return result;
    };

    double result = quad.integrate(ppred_integrand, 0.,
                                   std::numeric_limits<double>::infinity());

    BOOST_TEST(nd.logp(test_data) == log(result), tt::tolerance(1e-4));
  }
  BOOST_TEST(nd.N == 10);
}

BOOST_AUTO_TEST_CASE(logp_before_incorporate) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  BOOST_TEST(nd.logp(6.0) == -5.4563792395895785, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0., tt::tolerance(1e-6));

  nd.incorporate(5.0);
  nd.unincorporate(5.0);

  BOOST_TEST(nd.N == 0);
  BOOST_TEST(nd.logp(6.0) == -5.4563792395895785, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0., tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(sample) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  for (int i = 0; i < 1000; ++i) {
    nd.incorporate(42.0);
  }

  double s = nd.sample();

  BOOST_TEST(std::abs(s) < 100.0);
}

BOOST_AUTO_TEST_CASE(incorporate_raises_logp) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  double old_lp = nd.logp(10.0);
  for (int i = 0; i < 8; ++i) {
    nd.incorporate(10.0);
    double lp = nd.logp(10.0);
    BOOST_TEST(lp > old_lp);
    old_lp = lp;
  }
}

BOOST_AUTO_TEST_CASE(prior_prefers_origin) {
  std::mt19937 prng;
  ZeroMeanNormal nd1(&prng), nd2(&prng);

  for (int i = 0; i < 100; ++i) {
    nd1.incorporate(0.0);
    nd2.incorporate(50.0);
  }

  BOOST_TEST(nd1.logp_score() > nd2.logp_score());
}

BOOST_AUTO_TEST_CASE(transition_hyperparameters) {
  std::mt19937 prng;
  ZeroMeanNormal nd(&prng);

  nd.transition_hyperparameters();

  for (int i = 0; i < 100; ++i) {
    nd.incorporate(5.0);
  }

  nd.transition_hyperparameters();
  BOOST_TEST(nd.beta > 1.0);
}
