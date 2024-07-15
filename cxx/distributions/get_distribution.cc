// Copyright 2024
// See LICENSE.txt

#include <map>
#include <random>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "distributions/get_distribution.hh"

#include "util_parse.hh"

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "distributions/skellam.hh"
#include "distributions/stringcat.hh"

DistributionVariant get_distribution(
    const std::string& dist_string,
    std::mt19937* prng) {
  std::string dist_name;
  std::map<std::string, std::string> distribution_args;
  parse_name_and_parameters(dist_string, &dist_name, &distribution_args);

  if (dist_name == "bernoulli") {
    return new BetaBernoulli;
  } else if (dist_name == "bigram") {
    return new Bigram;
  } else if (dist_name == "categorical") {
    int num_categories = std::stoi(distribution_args.at("k"));
    return new DirichletCategorical(num_categories);
  } else if (dist_name == "normal") {
    return new Normal;
  } else if (dist_name == "skellam") {
    Skellam* s = new Skellam;
    s->init_theta(prng);
    return s;
  } else if (dist_name == "stringcat") {
    std::string delim = " ";  // Default deliminator
    auto it = distribution_args.find("delim");
    if (it != distribution_args.end()) {
      delim = it->second;
      assert(delim.length() == 1);
    }
    std::vector<std::string> strings;
    boost::split(strings, distribution_args.at("strings"),
                 boost::is_any_of(delim));
    return new StringCat(strings);
  }
  assert(false && "Unsupported distribution name.");
}

void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<bool>** dist_out) {
  *dist_out = std::get<Distribution<bool>*>(dv);
}

void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<double>** dist_out) {
  *dist_out = std::get<Distribution<double>*>(dv);
}

void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<int>** dist_out) {
  *dist_out = std::get<Distribution<int>*>(dv);
}

void get_distribution_from_distribution_variant(
    const DistributionVariant &dv, Distribution<std::string>** dist_out) {
  *dist_out = std::get<Distribution<std::string>*>(dv);
}
