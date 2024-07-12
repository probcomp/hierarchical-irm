// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Relation

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
  CleanRelation<bool> R1("R1", "bernoulli", {&D1, &D2});
  R1.incorporate(&prng, {0, 1}, 1);
  R1.incorporate(&prng, {1, 1}, 1);
  R1.incorporate(&prng, {3, 1}, 1);
  R1.incorporate(&prng, {4, 1}, 1);
  R1.incorporate(&prng, {5, 1}, 1);

  NoisyRelation<bool> NR1("NR1", "sometimes_bitflip", {&D1, &D2, &D3}, &R1);
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

  CleanRelation<std::string> R2("R2", "bigram", {&D2, &D3});
  NoisyRelation<std::string> NR2("NR2", "simple_string", {&D2, &D3}, &R2);

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
