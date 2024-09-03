// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test UtilMath

#include "util_math.hh"

#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

std::vector<double> check_linspace(double start, double stop, int num,
                                   bool endpoint, bool log_space) {
  std::vector<double> l;
  if (log_space) {
    l = log_linspace(start, stop, num, endpoint);
  } else {
    l = linspace(start, stop, num, endpoint);
  }
  BOOST_CHECK_CLOSE(l[0], start, 1e-6);
  if (endpoint) {
    BOOST_CHECK_CLOSE(l.back(), stop, 1e-6);
  }
  return l;
}

std::vector<double> check_log_linspace(double start, double stop, int num,
                                       bool endpoint) {
  auto l = check_linspace(start, stop, num, endpoint, /*log_space=*/true);
  return l;
}

BOOST_AUTO_TEST_CASE(test_lbeta) {
  // lbeta(x, y) = lbeta(y, x)
  BOOST_CHECK_CLOSE(lbeta(1, 2), lbeta(2, 1), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(2, 3), lbeta(3, 2), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(4, 7), lbeta(7, 4), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(3.14, 1.2), lbeta(1.2, 3.14), 1e-6);

  // -log(x) = lbeta(1, x)
  BOOST_CHECK_CLOSE(lbeta(1., 2), -log(2.), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(1., 10), -log(10.), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(1., 11), -log(11.), 1e-6);

  // lbeta(x + 1, y) = lbeta(x, y) + log(x / (x + y))
  BOOST_CHECK_CLOSE(lbeta(4, 2), lbeta(3, 2) + log(3. / (3 + 2)), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(3, 1), lbeta(2, 1) + log(2. / (2 + 1)), 1e-6);
  BOOST_CHECK_CLOSE(lbeta(5, 6), lbeta(4, 6) + log(4. / (4 + 6)), 1e-6);
}

BOOST_AUTO_TEST_CASE(test_linspace) {
  check_linspace(0., 0.2, 3, true, /*log_space=*/false);
  check_linspace(0., 0.2, 3, false, /*log_space=*/false);
  auto l = check_linspace(0., 1., 11, true, /*log_space=*/false);
  std::vector<double> expected_vector1 = {0.,  0.1, 0.2, 0.3, 0.4, 0.5,
                                          0.6, 0.7, 0.8, 0.9, 1.};
  BOOST_TEST(l == expected_vector1, tt::tolerance(1e-3) << tt::per_element());
  l = check_linspace(0., 1., 11, false, /*log_space=*/false);
  std::vector<double> expected_vector2 = {0.,      1 / 11., 2 / 11., 3 / 11.,
                                          4 / 11., 5 / 11., 6 / 11., 7 / 11.,
                                          8 / 11., 9 / 11., 10 / 11.};
  BOOST_TEST(l == expected_vector2, tt::tolerance(1e-3) << tt::per_element());
}

BOOST_AUTO_TEST_CASE(test_log_linspace) {
  check_log_linspace(0.1, 0.5, 3, true);
  check_log_linspace(0.1, 0.5, 3, false);
  check_log_linspace(0.1, 1.1, 11, false);
  check_log_linspace(0.1, 1.1, 11, true);
}

BOOST_AUTO_TEST_CASE(test_log_normalize) {
  std::vector<double> example1 = {-0.1, -5., -2., 1., 3.7, 4.};
  std::vector<double> normalized = log_normalize(example1);
  std::vector<double> exp_normalized;

  boost::range::transform(normalized, std::back_inserter(exp_normalized), exp);
  double sum = boost::accumulate(exp_normalized, 0.);
  BOOST_CHECK_CLOSE(sum, 1., 1e-6);

  std::vector<double> example2 = {-std::numeric_limits<double>::infinity(),
                                  -3.,
                                  -2.,
                                  0.4,
                                  0.6,
                                  1.,
                                  3.7,
                                  4.,
                                  8.};
  std::vector<double> normalized2 = log_normalize(example2);
  std::vector<double> exp_normalized2;

  boost::range::transform(normalized2, std::back_inserter(exp_normalized2),
                          exp);
  sum = boost::accumulate(exp_normalized2, 0.);
  BOOST_CHECK_CLOSE(sum, 1., 1e-6);
}

BOOST_AUTO_TEST_CASE(test_logsumexp) {
  double inf = std::numeric_limits<double>::infinity();
  double neginf = -std::numeric_limits<double>::infinity();
  BOOST_CHECK_CLOSE(logsumexp({-500., -501., -503.}), -499.65098778, 1e-6);
  BOOST_CHECK_CLOSE(logsumexp({-1000.}), -1000., 1e-6);
  BOOST_CHECK_CLOSE(logsumexp({-1000., -1000.}), -1000. + log(2.), 1e-6);
  BOOST_CHECK_CLOSE(logsumexp({-3., -3., -3., -3.}), -3. + log(4.), 1e-6);
  BOOST_CHECK_CLOSE(logsumexp({neginf, 1.}), 1., 1e-6);
  BOOST_TEST(logsumexp({neginf, neginf}) == neginf);
  BOOST_TEST(logsumexp({inf, inf}) == inf);
}

BOOST_AUTO_TEST_CASE(test_product) {
  std::vector<std::vector<int>> product_result = product({{1, 2}, {3, 4, 5}});
  std::vector<std::vector<int>> expected_result = {{1, 3}, {1, 4}, {1, 5},
                                                   {2, 3}, {2, 4}, {2, 5}};
  BOOST_TEST(expected_result.size() == 6);
  for (int i = 0; i < std::ssize(expected_result); ++i) {
    BOOST_TEST(product_result[i] == expected_result[i]);
  }
}

BOOST_AUTO_TEST_CASE(test_sample_from_logps_single) {
  // One of these entries isn't like the others, ...
  std::vector<double> logps = {-1.0, -2.0, 20.0, -3.0};
  std::mt19937 prng;
  BOOST_TEST(2 == sample_from_logps(logps, &prng));
}

BOOST_AUTO_TEST_CASE(test_sample_from_logps_all_small) {
  std::vector<double> logps = {-10.0, -20.0, -30.0, -40.0, -50.0};
  std::mt19937 prng;
  BOOST_TEST(0 == sample_from_logps(logps, &prng));
}

BOOST_AUTO_TEST_CASE(test_sample_from_logps) {
  std::vector<double> probs = {0.3, 0.1, 0.05, 0.1, 0.24, 0.11, 0., 0.1};
  std::vector<double> logps;
  for (int i = 0; i < std::ssize(probs); ++i) {
    // Ensure these are not normalized.
    logps.emplace_back(log(probs[i]) - 2.);
  }
  std::mt19937 prng;
  int num_samples = 100000;
  std::vector<int> counts{0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 0; i < num_samples; ++i) {
    ++counts[sample_from_logps(logps, &prng)];
  }
  for (int i = 0; i < std::ssize(logps); ++i) {
    double approx_p = (1. * counts[i]) / num_samples;
    double stddev = sqrt(probs[i] * (1 - probs[i]) / num_samples);
    BOOST_TEST(abs(probs[i] - approx_p) <= 3 * stddev);
  }
}
