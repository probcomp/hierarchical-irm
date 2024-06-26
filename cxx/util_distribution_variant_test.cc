// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test UtilDistributionVariant

#include "util_distribution_variant.hh"

#include <boost/test/included/unit_test.hpp>
#include <iostream>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "domain.hh"

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_parse_distribution_spec) {
  std::mt19937 prng;

  DistributionSpec dbb = parse_distribution_spec("bernoulli");
  BOOST_TEST((dbb.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(dbb.distribution_args.empty());

  DistributionSpec dbg = parse_distribution_spec("bigram");
  BOOST_TEST((dbg.distribution == DistributionEnum::bigram));
  BOOST_TEST(dbg.distribution_args.empty());

  DistributionSpec dn = parse_distribution_spec("normal");
  BOOST_TEST((dn.distribution == DistributionEnum::normal));
  BOOST_TEST(dn.distribution_args.empty());

  DistributionSpec ds = parse_distribution_spec("skellam");
  BOOST_TEST((ds.distribution == DistributionEnum::skellam));
  BOOST_TEST(ds.distribution_args.empty());

  DistributionSpec dc = parse_distribution_spec("categorical(k=6)");
  BOOST_TEST((dc.distribution == DistributionEnum::categorical));
  BOOST_TEST((dc.distribution_args.size() == 1));
  std::string expected = "6";
  BOOST_CHECK_EQUAL(dc.distribution_args.at("k"), expected);

  DistributionSpec dsc = parse_distribution_spec("stringcat(strings=a b c d)");
  BOOST_TEST((dsc.distribution == DistributionEnum::stringcat));
  BOOST_TEST((dsc.distribution_args.size() == 1));
  BOOST_CHECK_EQUAL(dsc.distribution_args.at("strings"), "a b c d");
  DistributionVariant dv = cluster_prior_from_spec(dsc, &prng);
  BOOST_TEST(std::get<StringCat*>(dv)->strings.size() == 4);

  DistributionSpec dsc2 = parse_distribution_spec(
      "stringcat(strings=yes:no,delim=:)");
  BOOST_TEST((dsc2.distribution == DistributionEnum::stringcat));
  BOOST_TEST((dsc2.distribution_args.size() == 2));
  BOOST_CHECK_EQUAL(dsc2.distribution_args.at("strings"), "yes:no");
  DistributionVariant dv2 = cluster_prior_from_spec(dsc2, &prng);
  BOOST_TEST(std::get<StringCat*>(dv2)->strings.size() == 2);
}

BOOST_AUTO_TEST_CASE(test_cluster_prior_from_spec) {
  std::mt19937 prng;
  DistributionSpec dc = {DistributionEnum::categorical, {{"k", "4"}}};
  DistributionVariant dcdv = cluster_prior_from_spec(dc, &prng);
  DirichletCategorical* dcd = std::get<DirichletCategorical*>(dcdv);
  BOOST_TEST(dcd->counts.size() == 4);
}
