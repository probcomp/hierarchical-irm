// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test TransitionLatentValue

#include "transition_latent_value.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>
#include <vector>

#include "clean_relation.hh"
#include "distributions/get_distribution.hh"
#include "domain.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_transition_latent_value) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  DistributionSpec base_spec("normal");
  EmissionSpec em_spec("sometimes_gaussian");

  CleanRelation<double> base_relation("base", base_spec, {&D1, &D2});
  std::vector<int> base_items = {1, 2};
  base_relation.incorporate(&prng, base_items, 1.2);
  base_relation.incorporate(&prng, {1, 1}, 0.8);
  base_relation.incorporate(&prng, {1, 3}, 0.7);

  NoisyRelation<double> NR1("NR1", em_spec, {&D1, &D2, &D3}, &base_relation);
  NR1.incorporate(&prng, {1, 2, 0}, 1.1);
  NR1.incorporate(&prng, {1, 2, 2}, 0.6);
  NR1.incorporate(&prng, {1, 1, 2}, 0.8);

  NoisyRelation<double> NR2("NR2", em_spec, {&D1, &D2}, &base_relation);
  NR2.incorporate(&prng, {1, 2}, 0.7);
  NR2.incorporate(&prng, {1, 3}, 1.3);

  transition_latent_value(&prng, base_items, &base_relation,
                          {{"NR1", &NR1}, {"NR2", &NR2}});

  // The latent value in the base relation has changed and is within the range
  // of the noisy observations.
  BOOST_TEST(base_relation.get_value(base_items) != 1.2);
  BOOST_TEST(base_relation.get_value(base_items) >= 0.6);
  BOOST_TEST(base_relation.get_value(base_items) <= 1.1);

  // Other values in the base relation have not changed.
  BOOST_TEST(base_relation.get_value({1, 1}) == 0.8);
  BOOST_TEST(base_relation.get_value({1, 3}) == 0.7);

  // The values in the noisy relations are unchanged.
  BOOST_TEST(NR1.get_value({1, 2, 0}) == 1.1);
  BOOST_TEST(NR1.get_value({1, 1, 2}) == 0.8);
  BOOST_TEST(NR2.get_value({1, 2}) == 0.7);
  BOOST_TEST(NR2.get_value({1, 3}) == 1.3);

  // The number of incorporated points in each relation has not changed.
  BOOST_TEST(base_relation.get_data().size() == 3);
  BOOST_TEST(NR1.get_data().size() == 3);
  BOOST_TEST(NR2.get_data().size() == 2);

  // Transitioning a latent value observed by only a subset of noisy relations
  // is supported. In this case, the new latent value must be that of the only
  // incorporated noisy observation.
  transition_latent_value(&prng, {1, 3}, &base_relation,
                          {{"NR1", &NR1}, {"NR2", &NR2}});
  BOOST_TEST(base_relation.get_value({1, 3}) == 1.3);
}

BOOST_AUTO_TEST_CASE(test_transition_latent_value_noisy_base) {
  std::mt19937 prng;
  Domain D1("D1");
  Domain D2("D2");
  Domain D3("D3");
  DistributionSpec base_spec("bigram");
  EmissionSpec em_spec("simple_string");

  CleanRelation<std::string> base_relation("base", base_spec, {&D1, &D2});
  std::vector<int> base_items = {1, 2};
  base_relation.incorporate(&prng, base_items, "apple");
  base_relation.incorporate(&prng, {1, 1}, "banana");
  base_relation.incorporate(&prng, {1, 3}, "pear");

  NoisyRelation<std::string> NR1("NR1", em_spec, {&D1, &D2}, &base_relation);
  NR1.incorporate(&prng, base_items, "aple");
  NR1.incorporate(&prng, {1, 3}, "peaar");

  NoisyRelation<std::string> NR2("NR2", em_spec, {&D1, &D2, &D3}, &NR1);
  NR2.incorporate(&prng, {1, 2, 0}, "aaple");
  NR2.incorporate(&prng, {1, 2, 2}, "apl");
  NR2.incorporate(&prng, {1, 3, 2}, "peaarr");
  transition_latent_value(&prng, base_items, &NR1, {{"NR2", &NR2}});

  // The latent value in the base relation has changed.
  BOOST_TEST(NR1.get_value(base_items) != "aple");

  // Other values in the base relation have not changed.
  BOOST_TEST(NR1.get_value({1, 3}) == "peaar");

  // The values in the noisy relations are unchanged.
  BOOST_TEST(NR2.get_value({1, 2, 0}) == "aaple");
  BOOST_TEST(NR2.get_value({1, 2, 2}) == "apl");

  // The number of incorporated points in each relation has not changed.
  BOOST_TEST(NR1.get_data().size() == 2);
  BOOST_TEST(NR2.get_data().size() == 3);
}
