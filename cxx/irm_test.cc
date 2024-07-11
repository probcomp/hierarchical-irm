// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test IRM

#include "irm.hh"

#include <boost/test/included/unit_test.hpp>

#include "util_distribution_variant.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_irm) {
  std::map<std::string, T_relation> schema1{
      {"R1", T_clean_relation{{"D1", "D1"}, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D1", "D2"}, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D3", "D1"}, DistributionSpec("bigram")}},
      {"R4",
       T_noisy_relation{
           {"D1", "D2", "D3"}, EmissionSpec("sometimes_gaussian"), "R2"}}};
  IRM irm(schema1);

  BOOST_TEST(irm.logp_score() == 0.0);

  std::mt19937 prng;
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == 0.0);

  irm.remove_relation("R3");

  auto obs0 = observation_string_to_value("0", ObservationEnum::bool_type);

  double logp_x = irm.logp({{"R1", {1, 2}, obs0}}, &prng);
  BOOST_TEST(logp_x < 0.0);

  irm.incorporate(&prng, "R1", {1, 2}, obs0);
  double one_obs_score = irm.logp_score();
  BOOST_TEST(one_obs_score < 0.0);

  // Transitioning clusters shouldn't change the score with only one
  // observation.
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == one_obs_score);

  // TODO(thomaswc):  Uncomment below when relation::unincorporate is
  // implemented.
  // irm.unincorporate("R1", {1, 2});
  // BOOST_TEST(irm.logp_score() == 0.0);

  irm.incorporate(&prng, "R2", {0, 3}, 1.);
  irm.incorporate(&prng, "R4", {0, 3, 1}, 1.2);
  irm.incorporate(&prng, "R4", {0, 3, 0}, 0.9);
  BOOST_TEST(irm.logp_score() < 0.0);

  // Transitioning clusters should preserve the number of observations.
  irm.transition_cluster_assignments_all(&prng);
  int r4_obs = std::visit([](auto r) { return r->get_data().size(); },
                          irm.relations.at("R4"));
  BOOST_TEST(r4_obs == 2);
}

BOOST_AUTO_TEST_CASE(test_irm_one_data_point) {
  // We have one data point {1, 1}, for which we compute different relations
  // against. Because each domain (D1, D2) has only one point, there is only one
  // cluster, and thus no marginalizing over different cluster assignments, so
  // logp_score = logp(x).
  std::map<std::string, T_relation> schema1{
      {"R1", T_clean_relation{{"D1"}, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D2"}, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D1", "D2"}, DistributionSpec("normal")}}};
  IRM irm(schema1);

  BOOST_TEST(irm.logp_score() == 0.0);

  std::mt19937 prng;
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == 0.0);

  auto obs0 = observation_string_to_value("0", ObservationEnum::bool_type);
  auto obs1 = observation_string_to_value("1", ObservationEnum::double_type);
  auto obs2 = observation_string_to_value("2", ObservationEnum::double_type);

  double logp_x = irm.logp({{"R1", {1,}, obs0}}, &prng);
  BOOST_TEST(logp_x < 0.0);

  double logp_y = irm.logp({{"R2", {23,}, obs1}}, &prng);
  BOOST_TEST(logp_y < 0.0);

  double logp_z = irm.logp({{"R3", {1, 23}, obs2}}, &prng);
  BOOST_TEST(logp_z < 0.0);

  // When we incorporate observations for a single domain entry, there is only
  // one latent cluster and nothing to marginalize over. Thus logp_score should
  // be the same as computing logp before incorporating.

  irm.incorporate(&prng, "R1", {1,}, obs0);
  double one_obs_score = irm.logp_score();
  BOOST_TEST(one_obs_score < 0.0);
  BOOST_TEST(one_obs_score == logp_x);

  irm.incorporate(&prng, "R2", {23,}, obs1);
  double two_obs_score = irm.logp_score();
  BOOST_TEST(two_obs_score < 0.0);
  BOOST_TEST(two_obs_score == (logp_x + logp_y));

  irm.incorporate(&prng, "R3", {1, 23}, obs2);
  double three_obs_score = irm.logp_score();
  BOOST_TEST(three_obs_score < 0.0);
  BOOST_TEST(three_obs_score == (logp_x + logp_y + logp_z));
}
