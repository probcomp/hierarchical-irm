// Copyright 2024
// See LICENSE.txt

#include "relation_variant.hh"

#include <cassert>

#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "domain.hh"
#include "relation.hh"

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
