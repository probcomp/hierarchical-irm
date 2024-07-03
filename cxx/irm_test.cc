// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test IRM

#include "irm.hh"

#include <boost/test/included/unit_test.hpp>

#include "util_distribution_variant.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_irm) {
  std::map<std::string, T_relation> schema1{
      {"R1",
       T_relation{{"D1", "D1"}, DistributionSpec{DistributionEnum::bernoulli}}},
      {"R2",
       T_relation{{"D1", "D2"}, DistributionSpec{DistributionEnum::normal}}},
      {"R3",
       T_relation{{"D3", "D1"}, DistributionSpec{DistributionEnum::bigram}}}};
  IRM irm(schema1);

  BOOST_TEST(irm.logp_score() == 0.0);

  std::mt19937 prng;
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == 0.0);

  irm.remove_relation("R3");

  auto obs0 = observation_string_to_value("0", DistributionEnum::bernoulli);

  double logp_x = irm.logp({{"R1", {1, 2}, obs0}}, &prng);
  BOOST_TEST(logp_x < 0.0);

  irm.incorporate(&prng, "R1", {1, 2}, obs0);
  double one_obs_score = irm.logp_score();
  BOOST_TEST(one_obs_score < 0.0);

  // TODO(thomaswc): Figure out why the next test doesn't pass.
  // BOOST_TEST(one_obs_score == logp_x);

  // Transitioning clusters shouldn't change the score with only one
  // observation.
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == one_obs_score);

  // TODO(thomaswc):  Uncomment below when relation::unincorporate is
  // implemented.
  // irm.unincorporate("R1", {1, 2});
  // BOOST_TEST(irm.logp_score() == 0.0);
}
