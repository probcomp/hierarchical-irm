// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test noisy_relation

#include "noisy_relation.hh"

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <random>

#include "clean_relation.hh"
#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "domain.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_noisy_relation) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  D1.incorporate(&prng, 0);
  D2.incorporate(&prng, 1);
  D3.incorporate(&prng, 3);
  DistributionSpec spec = DistributionSpec("bernoulli");
  CleanRelation<bool> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 1);
  R1.incorporate(&prng, {1, 1}, 1);
  R1.incorporate(&prng, {3, 1}, 1);
  R1.incorporate(&prng, {4, 1}, 1);
  R1.incorporate(&prng, {5, 1}, 1);

  EmissionSpec em_spec = EmissionSpec("sometimes_bitflip");
  NoisyRelation<bool> NR1("NR1", em_spec, {&D1, &D2, &D3}, &R1);
  NR1.incorporate(&prng, {0, 1, 3}, 0);
  NR1.incorporate(&prng, {1, 1, 3}, 1);
  NR1.incorporate(&prng, {3, 1, 3}, 0);
  NR1.incorporate(&prng, {4, 1, 3}, 1);
  NR1.incorporate(&prng, {5, 1, 3}, 0);
  NR1.incorporate(&prng, {0, 1, 4}, 1);
  NR1.incorporate(&prng, {0, 1, 6}, 0);
  auto z1 = NR1.get_cluster_assignment({0, 1, 3});
  BOOST_TEST(z1.size() == 3);
  BOOST_TEST(z1[0] == 0);
  BOOST_TEST(z1[1] == 0);
  BOOST_TEST(z1[2] == 0);

  double lpg __attribute__((unused));
  lpg = NR1.logp_gibbs_approx(D1, 0, 1, &prng);
  lpg = NR1.logp_gibbs_approx(D1, 0, 0, &prng);
  lpg = NR1.logp_gibbs_approx(D1, 0, 10, &prng);
  NR1.set_cluster_assignment_gibbs(D1, 0, 1, &prng);

  DistributionSpec bigram_spec = DistributionSpec("bigram");
  CleanRelation<std::string> R2("R2", bigram_spec, {&D2, &D3});
  EmissionSpec str_emspec = EmissionSpec("simple_string");
  NoisyRelation<std::string> NR2("NR2", str_emspec, {&D2, &D3}, &R2);

  R2.incorporate(&prng, {1, 3}, "cat");
  R2.incorporate(&prng, {2, 3}, "cat");
  R2.incorporate(&prng, {1, 2}, "dog");
  R2.incorporate(&prng, {2, 6}, "fish");

  NR2.incorporate(&prng, {1, 3}, "catt");
  NR2.incorporate(&prng, {2, 3}, "at");
  NR2.incorporate(&prng, {1, 2}, "doge");
  NR2.incorporate(&prng, {2, 6}, "fish");

  NR2.transition_cluster_hparams(&prng, 4);
  lpg = NR2.logp_gibbs_approx(D2, 2, 0, &prng);
  NR2.set_cluster_assignment_gibbs(D3, 3, 1, &prng);
  D1.set_cluster_assignment_gibbs(0, 1);
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

  EmissionSpec em_spec = EmissionSpec("sometimes_bitflip");
  NoisyRelation<bool> NR1("NR1", em_spec, {&D1, &D2}, &R1);

  NR1.incorporate(&prng, {0, 1}, 0);
  NR1.incorporate(&prng, {0, 2}, 1);
  NR1.incorporate(&prng, {3, 0}, 0);
  NR1.incorporate(&prng, {3, 1}, 1);

  NR1.unincorporate({3, 1});
  BOOST_TEST(NR1.data.size() == 3);
  BOOST_TEST(NR1.data.size() == 3);
  // Expect that these are still in the domain since the data points {3, 0} and
  // {0, 1} refer to them.
  BOOST_TEST(D1.items.contains(3));
  BOOST_TEST(D2.items.contains(1));

  NR1.unincorporate({0, 2});
  BOOST_TEST(NR1.data.size() == 2);
  BOOST_TEST(D1.items.contains(0));
  BOOST_TEST(!D2.items.contains(2));

  NR1.unincorporate({0, 1});
  BOOST_TEST(NR1.data.size() == 1);
  BOOST_TEST(!D1.items.contains(0));
  BOOST_TEST(!D2.items.contains(1));
}

BOOST_AUTO_TEST_CASE(test_get_update_value) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  DistributionSpec spec("bigram");
  CleanRelation<std::string> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, "chair");
  R1.incorporate(&prng, {0, 2}, "table");

  EmissionSpec em_spec("simple_string");
  NoisyRelation<std::string> NR1("NR1", em_spec, {&D1, &D2, &D3}, &R1);

  NR1.incorporate(&prng, {0, 1, 1}, "chir");
  NR1.incorporate(&prng, {0, 1, 5}, "Chair?!");
  NR1.incorporate(&prng, {0, 1, 2}, "chairrrr");

  BOOST_TEST(NR1.get_value({0, 1, 5}) == "Chair?!");

  double init_logp_score = NR1.logp_score();
  NR1.update_value({0, 1, 5}, "char");

  BOOST_TEST(NR1.get_value({0, 1, 5}) == "char");
  BOOST_TEST(NR1.logp_score() != init_logp_score);
}

BOOST_AUTO_TEST_CASE(test_incorporate_cluster) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  DistributionSpec spec("normal");
  CleanRelation<double> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 3.);
  R1.incorporate(&prng, {0, 2}, 2.8);

  EmissionSpec em_spec("sometimes_gaussian");
  NoisyRelation<double> NR1("NR1", em_spec, {&D1, &D2, &D3}, &R1);

  NR1.incorporate(&prng, {0, 1, 1}, 3.1);
  NR1.incorporate(&prng, {0, 1, 5}, 3.2);
  NR1.incorporate(&prng, {0, 1, 2}, 2.9);
  NR1.incorporate(&prng, {0, 1, 3}, 3.1);
  NR1.incorporate(&prng, {0, 1, 7}, 2.9);

  int init_data_size = NR1.get_data().size();
  int init_cluster_size = NR1.emission_relation.clusters.at(NR1.get_cluster_assignment({0, 1, 1}))->N;
  double init_logp_score = NR1.logp_score();
  NR1.unincorporate_from_cluster({0, 1, 1});
  NR1.incorporate_to_cluster({0, 1, 1}, 1.);

  // logp_score should change, but the number of points in the cluster should
  // stay the same.
  BOOST_TEST(init_logp_score != NR1.logp_score());
  BOOST_TEST(init_cluster_size ==
             NR1.emission_relation.clusters.at(NR1.get_cluster_assignment({0, 1, 1}))->N);
  BOOST_TEST(init_data_size = NR1.get_data().size());
}

BOOST_AUTO_TEST_CASE(test_cluster_logp_sample) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  DistributionSpec spec("normal");
  CleanRelation<double> R1("R1", spec, {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 3.);

  EmissionSpec em_spec("sometimes_gaussian");
  NoisyRelation<double> NR1("NR1", em_spec, {&D1, &D2, &D3}, &R1);

  NR1.incorporate(&prng, {0, 1, 1}, 3.1);
  NR1.incorporate(&prng, {0, 1, 5}, 3.2);
  NR1.incorporate(&prng, {0, 1, 2}, 2.9);

  double sample = NR1.sample_at_items(&prng, {0, 1, 2});
  double lp = NR1.cluster_or_prior_logp(&prng, {0, 1, 2}, sample);
  BOOST_TEST(lp < 0.0);
}
