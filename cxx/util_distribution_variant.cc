// Copyright 2024
// See LICENSE.txt

#include "util_distribution_variant.hh"

#include <boost/algorithm/string.hpp>
#include <cassert>
#include <sstream>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "distributions/skellam.hh"
#include "distributions/stringcat.hh"
#include "emissions/bitflip.hh"
#include "emissions/gaussian.hh"
#include "emissions/simple_string.hh"
#include "emissions/sometimes.hh"

ObservationVariant observation_string_to_value(
    const std::string& value_str, const DistributionEnum& distribution) {
  switch (distribution) {
    case DistributionEnum::normal:
      return std::stod(value_str);
    case DistributionEnum::bernoulli:
      return static_cast<bool>(std::stoi(value_str));
    case DistributionEnum::categorical:
    case DistributionEnum::skellam:
      return std::stoi(value_str);
    case DistributionEnum::bigram:
    case DistributionEnum::stringcat:
      return value_str;
    default:
      assert(false && "Unsupported distribution enum value.");
  }
}

DistributionSpec parse_distribution_spec(const std::string& dist_str) {
  std::map<std::string, DistributionEnum> dist_name_to_enum = {
      {"bernoulli", DistributionEnum::bernoulli},
      {"bigram", DistributionEnum::bigram},
      {"categorical", DistributionEnum::categorical},
      {"normal", DistributionEnum::normal},
      {"skellam", DistributionEnum::skellam},
      {"stringcat", DistributionEnum::stringcat}};
  std::string dist_name = dist_str.substr(0, dist_str.find('('));
  DistributionEnum dist = dist_name_to_enum.at(dist_name);

  std::string args_str = dist_str.substr(dist_name.length());
  if (args_str.empty()) {
    return DistributionSpec{dist};
  } else {
    assert(args_str[0] == '(');
    assert(args_str.back() == ')');
    args_str = args_str.substr(1, args_str.size() - 2);

    std::string part;
    std::istringstream iss{args_str};
    std::map<std::string, std::string> dist_args;
    while (std::getline(iss, part, ',')) {
      assert(part.find('=') != std::string::npos);
      std::string arg_name = part.substr(0, part.find('='));
      std::string arg_val = part.substr(part.find('=') + 1);
      dist_args[arg_name] = arg_val;
    }
    return DistributionSpec{dist, dist_args};
  }
}

DistributionVariant cluster_prior_from_spec(const DistributionSpec& spec,
                                            std::mt19937* prng) {
  switch (spec.distribution) {
    case DistributionEnum::bernoulli:
      return new BetaBernoulli;
    case DistributionEnum::bigram:
      return new Bigram;
    case DistributionEnum::categorical: {
      assert(spec.distribution_args.size() == 1);
      int num_categories = std::stoi(spec.distribution_args.at("k"));
      return new DirichletCategorical(num_categories);
    }
    case DistributionEnum::normal:
      return new Normal;
    case DistributionEnum::skellam: {
      Skellam* s = new Skellam;
      s->init_theta(prng);
      return s;
    }
    case DistributionEnum::stringcat: {
      std::string delim = " ";  // Default deliminator
      auto it = spec.distribution_args.find("delim");
      if (it != spec.distribution_args.end()) {
        delim = it->second;
        assert(delim.length() == 1);
      }
      std::vector<std::string> strings;
      boost::split(strings, spec.distribution_args.at("strings"),
                   boost::is_any_of(delim));
      return new StringCat(strings);
    }
    default:
      assert(false && "Unsupported distribution enum value.");
  }
}

EmissionVariant cluster_prior_from_spec(const EmissionSpec& spec,
                                        std::mt19937* prng) {
  return get_emission(spec.emission_name, spec.emission_args);
}
