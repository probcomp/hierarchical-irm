// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test get_distribution

#include <boost/test/included/unit_test.hpp>

#include "distributions/get_distribution.hh"

BOOST_AUTO_TEST_CASE(test_get_bernoulli) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bernoulli", &prng);
  BOOST_TEST(dv.index() == 0);
}

BOOST_AUTO_TEST_CASE(test_get_bernoulli2) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bernoulli()", &prng);
  BOOST_TEST(dv.index() == 0);
}

BOOST_AUTO_TEST_CASE(test_get_bigram) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bigram", &prng);
  BOOST_TEST(dv.index() == 3);
}

BOOST_AUTO_TEST_CASE(test_get_categorical) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("categorical(k=5)", &prng);
  BOOST_TEST(dv.index() == 2);
}

BOOST_AUTO_TEST_CASE(test_get_normal) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("normal", &prng);
  BOOST_TEST(dv.index() == 1);
}

BOOST_AUTO_TEST_CASE(test_get_skellam) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("skellam", &prng);
  BOOST_TEST(dv.index() == 2);
}

BOOST_AUTO_TEST_CASE(test_get_stringcat) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("stringcat(strings=a b c)", &prng);
  BOOST_TEST(dv.index() == 3);
}

BOOST_AUTO_TEST_CASE(test_get_stringcat2) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution(
      "stringcat(delim=:, strings=hello:world)", &prng);
  BOOST_TEST(dv.index() == 3);
}
