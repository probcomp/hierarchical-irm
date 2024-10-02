// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test CRP

#include "distributions/crp.hh"

#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/test/included/unit_test.hpp>

#include "util_math.hh"
namespace bm = boost::math;
namespace tt = boost::test_tools;
namespace bm = boost::math;

BOOST_AUTO_TEST_CASE(test_simple) {
  CRP crp;

  BOOST_TEST(crp.max_table() == 0);

  T_item cat = 1;
  T_item dog = 2;
  T_item fish = 3;
  T_item bat = 4;
  T_item hamster = 5;
  T_item snake = 6;

  crp.incorporate(bat, 3);
  crp.incorporate(cat, 0);
  crp.incorporate(dog, 0);
  crp.incorporate(fish, 1);
  crp.incorporate(hamster, 0);
  crp.incorporate(snake, 1);
  BOOST_TEST(crp.N == 6);
  BOOST_TEST(crp.max_table() == 3);

  crp.unincorporate(cat);
  BOOST_TEST(crp.N == 5);

  crp.incorporate(cat, 3);
  crp.unincorporate(dog);
  BOOST_TEST(crp.N == 5);

  crp.incorporate(dog, 3);

  // Table assignments are as expected from `incorporate` calls.
  BOOST_TEST(crp.assignments.at(bat) == 3);
  BOOST_TEST(crp.assignments.at(cat) == 3);
  BOOST_TEST(crp.assignments.at(dog) == 3);
  BOOST_TEST(crp.assignments.at(fish) == 1);
  BOOST_TEST(crp.assignments.at(hamster) == 0);
  BOOST_TEST(crp.assignments.at(snake) == 1);

  // Table contents are as expected.
  BOOST_TEST(crp.tables.size() == 3);
  BOOST_TEST(crp.tables.at(0).size() == 1);
  BOOST_TEST(crp.tables.at(0).contains(hamster));
  BOOST_TEST(crp.tables.at(1).size() == 2);
  BOOST_TEST(crp.tables.at(1).contains(fish));
  BOOST_TEST(crp.tables.at(1).contains(snake));
  BOOST_TEST(crp.tables.at(3).size() == 3);
  BOOST_TEST(crp.tables.at(3).contains(bat));
  BOOST_TEST(crp.tables.at(3).contains(cat));
  BOOST_TEST(crp.tables.at(3).contains(dog));

  // Table weights are as expected.
  std::unordered_map<int, double> tw = crp.tables_weights();
  BOOST_TEST(tw.size() == 4);  // Three populated tables and one new one.
  BOOST_TEST(tw[0] == crp.tables.at(0).size());
  BOOST_TEST(tw[1] == crp.tables.at(1).size());
  BOOST_TEST(tw[3] == crp.tables.at(3).size());
  BOOST_TEST(tw[4] == crp.alpha);

  // Table weights gibbs is as expected.
  std::unordered_map<int, double> twg = crp.tables_weights_gibbs(1);
  BOOST_TEST(tw[0] == twg[0]);
  BOOST_TEST(tw[1] == twg[1] + 1.);
  BOOST_TEST(tw[3] == twg[3]);
  BOOST_TEST(tw[4] == twg[4]);

  // We expect that this is log(table_size) - log(N + alpha).
  BOOST_TEST(crp.logp(1) == log(2. / (crp.alpha + crp.N)), tt::tolerance(1e-6));
  BOOST_TEST(crp.logp(0) == log(1. / (crp.alpha + crp.N)), tt::tolerance(1e-6));
  BOOST_TEST(crp.logp(3) == log(3. / (crp.alpha + crp.N)), tt::tolerance(1e-6));

  // This should be log(alpha ^ 3 * 0! * 2! * 1! * 0! / (6!)) = -log(360)
  BOOST_CHECK_CLOSE(crp.logp_score(), -log(360.), 1e-6);
}

BOOST_AUTO_TEST_CASE(test_log_prob) {
  CRP crp;

  T_item desk = 1;
  T_item chair = 2;
  T_item bureau = 3;
  T_item lamp = 4;
  T_item sofa = 5;

  crp.incorporate(desk, 3);
  double logp_score0 = crp.logp_score();
  // Only one configuration for a single item, so p_score==1.
  BOOST_TEST(logp_score0 == 0., tt::tolerance(1e-9));

  double log_cond1 = log(crp.alpha / (1. + crp.alpha));  // New cluster.
  crp.incorporate(chair, 2);
  double logp_score1 = crp.logp_score();
  // Successive log scores should equal the sum of the previous log score
  // and the conditional log prob of the next observation incorporated.
  BOOST_TEST(logp_score1 == logp_score0 + log_cond1, tt::tolerance(1e-9));

  double log_cond2 = crp.logp(3);
  crp.incorporate(bureau, 3);
  double logp_score2 = crp.logp_score();
  BOOST_TEST(logp_score2 == logp_score1 + log_cond2, tt::tolerance(1e-9));

  double log_cond3 = crp.logp(3);
  crp.incorporate(lamp, 3);
  double logp_score3 = crp.logp_score();
  BOOST_TEST(logp_score3 == logp_score2 + log_cond3, tt::tolerance(1e-9));

  double log_cond4 = crp.logp(2);
  crp.incorporate(sofa, 2);
  double logp_score4 = crp.logp_score();
  BOOST_TEST(logp_score4 == logp_score3 + log_cond4, tt::tolerance(1e-9));
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters) {
  std::mt19937 prng;
  CRP crp;

  crp.transition_alpha(&prng);
  double old_alpha = crp.alpha;

  for (int i = 0; i < 100; ++i) {
    crp.incorporate(i, 0);
  }

  crp.transition_alpha(&prng);
  // Expect that since all items are at one table, the new alpha is low.
  BOOST_TEST(crp.alpha < old_alpha);
}

BOOST_AUTO_TEST_CASE(test_crp_sample) {
  std::mt19937 prng;
  CRP crp;
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
    ++table_count[crp.sample(&prng)];
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
