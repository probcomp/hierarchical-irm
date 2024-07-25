// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test IRM

#include "irm.hh"

#include <boost/test/included/unit_test.hpp>
#include "distributions/get_distribution.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_irm) {
  std::map<std::string, T_relation> schema1{
      {"R1", T_clean_relation{{"D1", "D1"}, true, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D1", "D2"}, false, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D3", "D1"}, true, DistributionSpec("bigram")}},
      {"R4",
       T_noisy_relation{
           {"D1", "D2", "D3"}, true, EmissionSpec("sometimes_gaussian"), "R2"}}};
  IRM irm(schema1);

  BOOST_TEST(irm.logp_score() == 0.0);

  std::mt19937 prng;
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == 0.0);

  irm.remove_relation("R3");

  double logp_x = irm.logp({{"R1", {1, 2}, false}}, &prng);
  BOOST_TEST(logp_x < 0.0);

  irm.incorporate(&prng, "R1", {1, 2}, false);
  double one_obs_score = irm.logp_score();
  BOOST_TEST(one_obs_score < 0.0);

  // Transitioning clusters shouldn't change the score with only one
  // observation.
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == one_obs_score);

  irm.unincorporate("R1", {1, 2});
  BOOST_TEST(irm.logp_score() == 0.0);

  irm.incorporate(&prng, "R2", {0, 3}, 1.);
  irm.incorporate(&prng, "R4", {0, 3, 1}, 1.2);
  irm.incorporate(&prng, "R4", {0, 3, 0}, 0.9);
  BOOST_TEST(irm.logp_score() < 0.0);

  // Transitioning clusters should preserve the number of observations.
  irm.transition_cluster_assignments_all(&prng);
  int r4_obs = std::visit([](auto r) { return r->get_data().size(); },
                          irm.relations.at("R4"));
  BOOST_TEST(r4_obs == 2);

  // Transitioning the latent value in R2 should change it.
  irm.transition_latent_values_relation(&prng, "R2");
  Relation<double>* R2 = std::get<Relation<double>*>(irm.get_relation("R2"));
  BOOST_TEST(R2->get_data().at({0, 3}) != 1.);
}

BOOST_AUTO_TEST_CASE(test_irm_one_data_point) {
  // We have one data point {1, 1}, for which we compute different relations
  // against. Because each domain (D1, D2) has only one point, there is only one
  // cluster, and thus no marginalizing over different cluster assignments, so
  // logp_score = logp(x).
  std::map<std::string, T_relation> schema1{
      {"R1", T_clean_relation{{"D1"}, true, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D2"}, true, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D1", "D2"}, true, DistributionSpec("normal")}}};
  IRM irm(schema1);

  BOOST_TEST(irm.logp_score() == 0.0);

  std::mt19937 prng;
  irm.transition_cluster_assignments_all(&prng);
  BOOST_TEST(irm.logp_score() == 0.0);

  bool obs0 = false;
  double obs1 = 1.;
  double obs2 = 2.;

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

void construct_test_irm(std::mt19937* prng, IRM* irm) {
  irm->domains["D1"]->incorporate(prng, 0, 0);
  irm->domains["D1"]->incorporate(prng, 1, 0);
  irm->domains["D1"]->incorporate(prng, 2, 0);
  irm->domains["D1"]->incorporate(prng, 3, 1);

  irm->domains["D2"]->incorporate(prng, 0, 0);
  irm->domains["D2"]->incorporate(prng, 1, 0);
  irm->domains["D2"]->incorporate(prng, 2, 1);
  irm->domains["D2"]->incorporate(prng, 3, 2);

  irm->incorporate(prng, "R1", {0,}, false);
  irm->incorporate(prng, "R1", {1,}, false);
  irm->incorporate(prng, "R1", {2,}, true);
  irm->incorporate(prng, "R1", {3,}, true);

  irm->incorporate(prng, "R2", {0,}, 0.);
  irm->incorporate(prng, "R2", {1,}, 0.);
  irm->incorporate(prng, "R2", {2,}, 1.);
  irm->incorporate(prng, "R2", {3,}, 1.1);

  irm->incorporate(prng, "R3", {0, 2}, 0.);
  irm->incorporate(prng, "R3", {1, 1}, 0.);
  irm->incorporate(prng, "R3", {2, 2}, 1.);
  irm->incorporate(prng, "R3", {1, 3}, 1.1);
}

BOOST_AUTO_TEST_CASE(test_irm_logp_logp_score_agreement) {
  std::map<std::string, T_relation> schema{
      {"R1", T_clean_relation{{"D1"}, true, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D2"}, true, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D1", "D2"}, true, DistributionSpec("normal")}}};

  std::mt19937 prng;
  IRM irm(schema);
  construct_test_irm(&prng, &irm);

  // Now we would like to add the observation for {4, 4}.
  double logp_x = irm.logp({{"R3", {4, 4}, 0.5}}, &prng);

  // If we want to compare against logp_score, we need to compute logp_score
  // for each way we incorporate this observation.
  std::vector<double> logps;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 4; ++j) {
      IRM test_irm(schema);
      construct_test_irm(&prng, &test_irm);
      double logp0 = test_irm.logp_score();
      test_irm.domains["D1"]->incorporate(&prng, 4, i);
      test_irm.domains["D2"]->incorporate(&prng, 4, j);
      test_irm.incorporate(&prng, "R3", {4, 4}, 0.5);
      logps.push_back(test_irm.logp_score() - logp0);
    }
  }
  BOOST_TEST(logsumexp(logps) == logp_x, tt::tolerance(1e-6));
}
