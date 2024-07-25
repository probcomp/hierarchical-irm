// Copyright 2024
// See LICENSE.txt

#include "get_distribution.hh"

#include <boost/algorithm/string.hpp>
#include <cassert>
#include <sstream>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "distributions/skellam.hh"
#include "distributions/stringcat.hh"
#include "util_observation.hh"

DistributionSpec::DistributionSpec(
    const std::string& dist_str,
    const std::map<std::string, std::string>& _distribution_args):
  distribution_args(_distribution_args) {
  std::string dist_name = dist_str.substr(0, dist_str.find('('));
  std::string args_str = dist_str.substr(dist_name.length());
  if (!args_str.empty()) {
    assert(args_str[0] == '(');
    assert(args_str.back() == ')');
    args_str = args_str.substr(1, args_str.size() - 2);

    std::string part;
    std::istringstream iss{args_str};
    while (std::getline(iss, part, ',')) {
      assert(part.find('=') != std::string::npos);
      std::string arg_name = part.substr(0, part.find('='));
      std::string arg_val = part.substr(part.find('=') + 1);
      distribution_args[arg_name] = arg_val;
    }
  }
  if (dist_name == "bernoulli") {
    distribution = DistributionEnum::bernoulli;
    observation_type = ObservationEnum::bool_type;
  } else if (dist_name == "bigram") {
    distribution = DistributionEnum::bigram;
    observation_type = ObservationEnum::string_type;
  } else if (dist_name == "categorical") {
    distribution = DistributionEnum::categorical;
    observation_type = ObservationEnum::int_type;
  } else if (dist_name == "normal") {
    distribution = DistributionEnum::normal;
    observation_type = ObservationEnum::double_type;
  } else if (dist_name == "skellam") {
    distribution = DistributionEnum::skellam;
    observation_type = ObservationEnum::int_type;
  } else if (dist_name == "stringcat") {
    distribution = DistributionEnum::stringcat;
    observation_type = ObservationEnum::string_type;
  } else {
    assert(false && "Unsupported distribution name.");
  }
}

DistributionVariant get_prior(const DistributionSpec& spec,
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
