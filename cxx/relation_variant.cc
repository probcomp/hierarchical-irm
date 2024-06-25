// Copyright 2024
// See LICENSE.txt

#include "relation_variant.hh"

#include <cassert>

#include "domain.hh"
#include "relation.hh"

RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains) {
  DistributionVariant dv = cluster_prior_from_spec(dist_spec);

  RelationVariant rv;

  std::visit(
      [&](const auto& v) {
      rv = new Relation<decltype(*v)::SampleType>(
          name, dist_spec, domains);
      }, dv);

  return rv;
}
