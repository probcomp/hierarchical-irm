// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test HIRM

#include "hirm.hh"

#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_crp) {
  std::mt19937 prng;
  auto crp = CRP(&prng);
  double alpha = 1.;
  crp.incorporate(0, 0);
  crp.incorporate(1, 1);
  crp.incorporate(2, 1);
  crp.incorporate(3, 0);
  crp.incorporate(4, 0);
  crp.incorporate(5, 2);
  // Check that the table weights reflect the number of people at each table.
  auto weights = crp.tables_weights();
  BOOST_CHECK_CLOSE(weights[0], 3., 1e-6);
  BOOST_CHECK_CLOSE(weights[1], 2., 1e-6);
  BOOST_CHECK_CLOSE(weights[2], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[3], alpha, 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(0), log(3. / (6. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(1), log(2. / (6. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(2), log(1. / (6. + alpha)), 1e-6);

  // Test the CRP distribution
  // This should be log(alpha ^ 3 * 0! * 2! * 1! * 0! / (6!)) = -log(360)
  BOOST_CHECK_CLOSE(crp.logp_score(), -log(360.), 1e-6);

  // Delete two people and check again.
  crp.unincorporate(0);
  crp.unincorporate(2);

  weights = crp.tables_weights();
  BOOST_CHECK_CLOSE(weights[0], 2., 1e-6);
  BOOST_CHECK_CLOSE(weights[1], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[2], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[3], alpha, 1e-6);

  BOOST_CHECK_CLOSE(crp.logp(0), log(2. / (4. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(1), log(1. / (4. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(2), log(1. / (4. + alpha)), 1e-6);

  // This should be log(alpha ^ 3 * 0! * 1! * 0! * 0! / (4!)) = -log(24)
  BOOST_CHECK_CLOSE(crp.logp_score(), -log(24.), 1e-6);
}

BOOST_AUTO_TEST_CASE(test_crp_sample) {
  std::mt19937 prng;
  auto crp = CRP(&prng);
  for (int i = 0; i < 10; ++i) {
    crp.incorporate(i, 0);
  }
  for (int i = 10; i < 15; ++i) {
    crp.incorporate(i, 1);
  }

  // We have the following set up, 10 in the first table, and 5 in the second
  // table. This corresponds to a new customer having probability 10 / 16 for
  // the first table, 5 / 16 for the second table, and 1 / 16 for the next
  // table. Check that these frequencies are approximately matched.
  std::map<int, int> table_count;
  const int num_draws = 3000;
  for (int i = 0; i < num_draws; ++i) {
    ++table_count[crp.sample()];
  }

  // Check that the count of 0's is close to 10/16 = 5/8.
  // Because these are independent bernoulli draws, we check that we are within
  // one standard deviation using the Binomial stddev.
  BOOST_TEST(table_count[0] / static_cast<double>(num_draws) <=
             5 / 8. + sqrt(5 / 8. * 3 / 8. / num_draws));
  BOOST_TEST(table_count[0] / static_cast<double>(num_draws) >=
             5 / 8. - sqrt(5 / 8. * 3 / 8. / num_draws));

  // Check that the count of 1's is close to 5/16.
  BOOST_TEST(table_count[1] / static_cast<double>(num_draws) <=
             5 / 16. + sqrt(5 / 16. * 13 / 16. / num_draws));
  BOOST_TEST(table_count[1] / static_cast<double>(num_draws) >=
             5 / 16. - sqrt(5 / 16. * 13 / 16. / num_draws));

  // Check that the count of 2's is close to 1/16.
  BOOST_TEST(table_count[2] / static_cast<double>(num_draws) <=
             1 / 16. + sqrt(1 / 16. * 15 / 16. / num_draws));
  BOOST_TEST(table_count[2] / static_cast<double>(num_draws) >=
             1 / 16. - sqrt(1 / 16. * 15 / 16. / num_draws));
}
