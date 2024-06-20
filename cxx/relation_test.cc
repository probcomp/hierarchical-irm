// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Relation

#include "relation.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "domain.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_relation) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  D1.incorporate(&prng, 0);
  D2.incorporate(&prng, 1);
  D3.incorporate(&prng, 3);
  DistributionSpec spec = DistributionSpec{DistributionEnum::bernoulli};
  Relation<BetaBernoulli> R1("R1", spec, {&D1, &D2, &D3});
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

  double lpg __attribute__ ((unused));
  lpg = R1.logp_gibbs_approx(D1, 0, 1);
  lpg = R1.logp_gibbs_approx(D1, 0, 0);
  lpg = R1.logp_gibbs_approx(D1, 0, 10);
  R1.set_cluster_assignment_gibbs(D1, 0, 1);

  DistributionSpec bigram_spec = DistributionSpec{DistributionEnum::bigram};
  Relation<Bigram> R2("R1", bigram_spec, {&D2, &D3});
  R2.incorporate(&prng, {1, 3}, "cat");
  R2.incorporate(&prng, {1, 2}, "dog");
  R2.incorporate(&prng, {1, 4}, "catt");
  R2.incorporate(&prng, {2, 6}, "fish");

  lpg = R2.logp_gibbs_approx(D2, 2, 0);
  R2.set_cluster_assignment_gibbs(D3, 3, 1);
  D1.set_cluster_assignment_gibbs(0, 1);
}
