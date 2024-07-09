// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Normal

#include "distributions/normal.hh"

#include <boost/math/distributions/inverse_gamma.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/quadrature/gauss_kronrod.hpp>
#include <boost/test/included/unit_test.hpp>

#include "util_math.hh"
namespace bm = boost::math;
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_welford) {
  Normal nd;
  std::vector<double> entries{-1., 2., 4., 5., 6., 20.};
  std::vector<double> expected_means{-1., 0.5, 1 + 2 / 3., 2.5, 3.2, 6.};
  std::vector<double> expected_vars{0.,   2.25, 4 + 2 / 9.,
                                    5.25, 6.16, 44. + 1 / 3.};
  for (int i = 0; i < std::ssize(entries); ++i) {
    nd.incorporate(entries[i]);
    BOOST_TEST(nd.mean == expected_means[i], tt::tolerance(1e-6));
    BOOST_TEST(nd.var == expected_vars[i], tt::tolerance(1e-6));
  }

  for (int i = std::ssize(entries) - 1; i > 0; --i) {
    nd.unincorporate(entries[i]);
    BOOST_TEST(nd.mean == expected_means[i - 1], tt::tolerance(1e-6));
    BOOST_TEST(nd.var == expected_vars[i - 1], tt::tolerance(1e-6));
  }
}

BOOST_AUTO_TEST_CASE(test_log_prob) {
  Normal nd;

  bm::inverse_gamma_distribution inv_gamma_dist(nd.v / 2., nd.s / 2.);
  auto quad = bm::quadrature::gauss_kronrod<double, 100>();

  for (int i = 0; i < 10; ++i) {
    nd.incorporate(i);

    auto integrand1 = [&nd, &inv_gamma_dist, &i](double n, double ig) {
      bm::normal_distribution normal_prior_dist(nd.m, sqrt(ig / nd.r));
      bm::normal_distribution normal_dist(n, sqrt(ig));
      double result =
          bm::pdf(normal_prior_dist, n) * bm::pdf(inv_gamma_dist, ig);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(normal_dist, j);
      }
      return result;
    };

    auto integrand2 = [&quad, &integrand1](double ig) {
      auto f = [&](double n) { return integrand1(n, ig); };
      return quad.integrate(f, -std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::infinity());
    };

    double result =
        quad.integrate(integrand2, 0., std::numeric_limits<double>::infinity());

    BOOST_TEST(nd.logp_score() == log(result), tt::tolerance(1e-4));
  }
  BOOST_TEST(nd.N == 10);
}

BOOST_AUTO_TEST_CASE(test_posterior_pred) {
  Normal nd;

  bm::inverse_gamma_distribution inv_gamma_dist(nd.v / 2., nd.s / 2.);
  auto quad = bm::quadrature::gauss<double, 100>();

  for (int i = 0; i < 10; ++i) {
    double test_data = i / 3.;
    nd.incorporate(i);

    auto unnormalized = [&nd, &inv_gamma_dist, &i](double n, double ig) {
      bm::normal_distribution normal_prior_dist(nd.m, sqrt(ig / nd.r));
      bm::normal_distribution normal_dist(n, sqrt(ig));
      double result =
          bm::pdf(normal_prior_dist, n) * bm::pdf(inv_gamma_dist, ig);
      for (int j = 0; j <= i; ++j) {
        result *= bm::pdf(normal_dist, j);
      }
      return result;
    };

    auto ppred_integrand1 = [&unnormalized, &test_data, &nd](double n,
                                                             double ig) {
      double result = unnormalized(n, ig);
      double normalization = exp(nd.logp_score());
      // Compute p(theta | data) = p(data | theta) p(theta) / p(data)
      result /= normalization;
      bm::normal_distribution normal_dist(n, sqrt(ig));
      return bm::pdf(normal_dist, test_data) * result;
    };

    auto ppred_integrand2 = [&quad, &ppred_integrand1](double ig) {
      auto f = [&](double n) { return ppred_integrand1(n, ig); };
      return quad.integrate(f, -std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::infinity());
    };
    double result = quad.integrate(ppred_integrand2, 0.,
                                   std::numeric_limits<double>::infinity());

    BOOST_TEST(nd.logp(test_data) == log(result), tt::tolerance(1e-4));
  }
  BOOST_TEST(nd.N == 10);
}

BOOST_AUTO_TEST_CASE(simple) {
  Normal nd;

  nd.incorporate(5.0);
  nd.incorporate(-2.0);
  BOOST_TEST(nd.N == 2);

  nd.unincorporate(5.0);
  nd.incorporate(7.0);
  BOOST_TEST(nd.N == 2);

  nd.unincorporate(-2.0);
  BOOST_TEST(nd.N == 1);

  nd.incorporate(3.14, 2.71);
  BOOST_TEST(nd.N == 3.71);
}

BOOST_AUTO_TEST_CASE(no_nan_after_incorporate_unincorporate) {
  Normal nd;

  nd.incorporate(10.0);
  nd.unincorporate(10.0);

  BOOST_TEST(nd.N == 0);
  BOOST_TEST(!std::isnan(nd.mean));
  BOOST_TEST(!std::isnan(nd.var));
}

BOOST_AUTO_TEST_CASE(logp_before_incorporate) {
  Normal nd;

  BOOST_TEST(nd.logp(6.0) == -4.4357424552958129, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0.0, tt::tolerance(1e-6));

  nd.incorporate(5.0);
  nd.unincorporate(5.0);

  BOOST_TEST(nd.N == 0);
  BOOST_TEST(nd.logp(6.0) == -4.4357424552958129, tt::tolerance(1e-6));
  BOOST_TEST(nd.logp_score() == 0.0, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(sample) {
  std::mt19937 prng;
  Normal nd;

  for (int i = 0; i < 1000; ++i) {
    nd.incorporate(42.0);
  }

  double s = nd.sample(&prng);

  BOOST_TEST(s > 40.0);
  BOOST_TEST(s < 44.0);
}

BOOST_AUTO_TEST_CASE(incorporate_raises_logp) {
  Normal nd;

  double old_lp = nd.logp(10.0);
  for (int i = 0; i < 10; ++i) {
    nd.incorporate(10.0);
    double lp = nd.logp(10.0);
    BOOST_TEST(lp > old_lp);
    old_lp = lp;
  }
}

BOOST_AUTO_TEST_CASE(prior_prefers_origin) {
  Normal nd1, nd2;

  for (int i = 0; i < 100; ++i) {
    nd1.incorporate(0.0);
    nd2.incorporate(50.0);
  }

  BOOST_TEST(nd1.logp_score() > nd2.logp_score());
}

BOOST_AUTO_TEST_CASE(transition_hyperparameters) {
  std::mt19937 prng;
  Normal nd;

  nd.transition_hyperparameters(&prng);

  for (int i = 0; i < 100; ++i) {
    nd.incorporate(5.0);
  }

  nd.transition_hyperparameters(&prng);
  BOOST_TEST(nd.m > 0.0);
}
