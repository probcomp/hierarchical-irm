// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test get_distribution

#include <string.h>
#include <typeinfo>

#include <boost/test/included/unit_test.hpp>

#include "distributions/get_distribution.hh"

BOOST_AUTO_TEST_CASE(test_get_bernoulli) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bernoulli", &prng);
  Distribution<bool> *d = std::get<Distribution<bool>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("BetaBernoulli") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_bernoulli2) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bernoulli()", &prng);
  Distribution<bool> *d = std::get<Distribution<bool>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("BetaBernoulli") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_bigram) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("bigram", &prng);
  Distribution<std::string> *d = std::get<Distribution<std::string>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Bigram") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_categorical) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("categorical(k=5)", &prng);
  Distribution<int> *d = std::get<Distribution<int>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("DirichletCategorical") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_normal) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("normal", &prng);
  Distribution<double> *d = std::get<Distribution<double>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Normal") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_skellam) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("skellam", &prng);
  Distribution<int> *d = std::get<Distribution<int>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("Skellam") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_stringcat) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("stringcat(strings=a b c)", &prng);
  Distribution<std::string> *d = std::get<Distribution<std::string>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("StringCat") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_stringcat2) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution(
      "stringcat(delim=:, strings=hello:world)", &prng);
  Distribution<std::string> *d = std::get<Distribution<std::string>*>(dv);
  std::string name = typeid(*d).name();
  BOOST_TEST(name.find("StringCat") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_get_distribution_from_distribution_variant) {
  std::mt19937 prng;
  DistributionVariant dv = get_distribution("normal", &prng);
  Distribution<double> *d;
  get_distribution_from_distribution_variant(dv, &d);
  d->incorporate(5.0);
  BOOST_TEST(d->N == 1);
}
