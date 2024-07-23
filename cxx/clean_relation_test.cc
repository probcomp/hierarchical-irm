// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Relation

#include "clean_relation.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "domain.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_clean_relation) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  D1.incorporate(&prng, 0);
  D2.incorporate(&prng, 1);
  D3.incorporate(&prng, 3);
  DistributionSpec spec = DistributionSpec("bernoulli");
  CleanRelation<bool> R1("R1", spec, {&D1, &D2, &D3});
  R1.incorporate(&prng, {0, 1, 3}, 1);
  R1.incorporate(&prng, {1, 1, 3}, 1);
  R1.incorporate(&prng, {3, 1, 3}, 1);
  R1.incorporate(&prng, {4, 1, 3}, 1);
  R1.incorporate(&prng, {5, 1, 3}, 1);
  R1.incorporate(&prng, {0, 1, 4}, 0);
  R1.incorporate(&prng, {0, 1, 6}, 1);
  auto z1 = R1.get_cluster_assignment({0, 1, 3});
  BOOST_TEST(z1.size() == 3);
  BOOST_TEST(z1[0] == 0);
  BOOST_TEST(z1[1] == 0);
  BOOST_TEST(z1[2] == 0);

  auto z2 = R1.get_cluster_assignment_gibbs({0, 1, 3}, D2, 1, 191);
  BOOST_TEST(z2.size() == 3);
  BOOST_TEST(z2[0] == 0);
  BOOST_TEST(z2[1] == 191);
  BOOST_TEST(z2[2] == 0);

  double lpg __attribute__((unused));
  lpg = R1.logp_gibbs_approx(D1, 0, 1, &prng);
  lpg = R1.logp_gibbs_approx(D1, 0, 0, &prng);
  lpg = R1.logp_gibbs_approx(D1, 0, 10, &prng);
  R1.set_cluster_assignment_gibbs(D1, 0, 1, &prng);

  Distribution<bool>* db = R1.make_new_distribution(&prng);
  BOOST_TEST(db->N == 0);
  db->incorporate(false);
  BOOST_TEST(db->N == 1);
}

BOOST_AUTO_TEST_CASE(test_string_relation) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");

  DistributionSpec bigram_spec = DistributionSpec("bigram");
  CleanRelation<std::string> R2("R2", bigram_spec, {&D1, &D2});
  R2.incorporate(&prng, {1, 3}, "cat");
  R2.incorporate(&prng, {1, 2}, "dog");
  R2.incorporate(&prng, {1, 4}, "catt");
  R2.incorporate(&prng, {2, 6}, "fish");

  double lpg __attribute__((unused));
  lpg = R2.logp_gibbs_approx(D2, 2, 0, &prng);
  R2.set_cluster_assignment_gibbs(D2, 3, 1, &prng);
  D1.set_cluster_assignment_gibbs(1, 1);

  Distribution<std::string>* db2 = R2.make_new_distribution(&prng);
  BOOST_TEST(db2->N == 0);
  db2->incorporate("hello");
  BOOST_TEST(db2->N == 1);
}

BOOST_AUTO_TEST_CASE(test_unincorporate) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  DistributionSpec spec = DistributionSpec("bernoulli");
  CleanRelation<bool> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 1);
  R1.incorporate(&prng, {0, 2}, 1);
  R1.incorporate(&prng, {3, 0}, 1);
  R1.incorporate(&prng, {3, 1}, 1);

  R1.unincorporate({3, 1});
  BOOST_TEST(R1.data.size() == 3);
  // Expect that these are still in the domain since the data points {3, 0} and
  // {0, 1} refer to them.
  BOOST_TEST(D1.items.contains(3));
  BOOST_TEST(D2.items.contains(1));

  R1.unincorporate({0, 2});
  BOOST_TEST(R1.data.size() == 2);
  BOOST_TEST(D1.items.contains(0));
  BOOST_TEST(!D2.items.contains(2));

  R1.unincorporate({0, 1});
  BOOST_TEST(R1.data.size() == 1);
  BOOST_TEST(!D1.items.contains(0));
  BOOST_TEST(!D2.items.contains(1));
}

BOOST_AUTO_TEST_CASE(test_emission) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  EmissionSpec spec = EmissionSpec("sometimes_bitflip");
  CleanRelation<std::pair<bool, bool>> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, {1, 1});
  BOOST_TEST(R1.data.size() == 1);

  double lp = R1.cluster_or_prior_logp(&prng, {0, 2}, {1, 1});
  BOOST_TEST(lp < 0.0);
}

BOOST_AUTO_TEST_CASE(test_get_update_value) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  DistributionSpec spec("bigram");
  CleanRelation<std::string> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, "chair");
  R1.incorporate(&prng, {0, 2}, "table");
  R1.incorporate(&prng, {5, 2}, "lamp");
  R1.incorporate(&prng, {1, 1}, "desk");

  BOOST_TEST(R1.get_value({5, 2}) == "lamp");

  double init_logp_score = R1.logp_score();
  R1.update_value({5, 2}, "carpet");

  BOOST_TEST(R1.get_value({5, 2}) == "carpet");
  BOOST_TEST(R1.logp_score() != init_logp_score);
}

BOOST_AUTO_TEST_CASE(test_incorporate_cluster) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  DistributionSpec spec("normal");
  CleanRelation<double> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 3.);
  R1.incorporate(&prng, {0, 2}, 2.8);
  R1.incorporate(&prng, {5, 2}, 2.3);

  int init_data_size = R1.get_data().size();
  int init_cluster_size = R1.clusters.at(R1.get_cluster_assignment({5, 2}))->N;
  double init_logp_score = R1.logp_score();
  R1.unincorporate_from_cluster({5, 2});
  R1.incorporate_to_cluster({5, 2}, 1.);

  // logp_score should change, but the number of points in the cluster should
  // stay the same.
  BOOST_TEST(init_logp_score != R1.logp_score());
  BOOST_TEST(init_cluster_size ==
             R1.clusters.at(R1.get_cluster_assignment({5, 2}))->N);
  BOOST_TEST(init_data_size = R1.get_data().size());
}

BOOST_AUTO_TEST_CASE(test_cluster_logp_sample) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  DistributionSpec spec("normal");
  CleanRelation<double> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 3.);
  R1.incorporate(&prng, {0, 2}, 2.8);
  R1.incorporate(&prng, {5, 2}, 2.3);

  double sample = R1.sample_at_items(&prng, {0, 2});
  double lp = R1.cluster_or_prior_logp(&prng, {0, 2}, sample);
  BOOST_TEST(lp < 0.0);
}
