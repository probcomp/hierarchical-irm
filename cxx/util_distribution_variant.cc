// Copyright 2024
// See LICENSE.txt

#include "util_distribution_variant.hh"

#include <cassert>
#include <sstream>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/crp.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "domain.hh"
#include "relation.hh"

ObservationVariant observation_string_to_value(
    const std::string& value_str, const DistributionEnum& distribution) {
  switch (distribution) {
    case DistributionEnum::normal:
      return std::stod(value_str);
    case DistributionEnum::bernoulli:
      return static_cast<bool>(std::stoi(value_str));
    case DistributionEnum::categorical:
      return std::stoi(value_str);
    case DistributionEnum::bigram:
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
      {"normal", DistributionEnum::normal}};
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

DistributionVariant cluster_prior_from_spec(const DistributionSpec& spec) {
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
    default:
      assert(false && "Unsupported distribution enum value.");
  }
}

RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains) {
  switch (dist_spec.distribution) {
    case DistributionEnum::bernoulli:
      return new Relation<BetaBernoulli>(name, dist_spec, domains);
    case DistributionEnum::bigram:
      return new Relation<Bigram>(name, dist_spec, domains);
    case DistributionEnum::categorical:
      return new Relation<DirichletCategorical>(name, dist_spec, domains);
    case DistributionEnum::normal:
      return new Relation<Normal>(name, dist_spec, domains);
    default:
      assert(false && "Unsupported distribution enum value.");
  }
}
