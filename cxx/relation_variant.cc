// Copyright 2024
// See LICENSE.txt

#include "relation_variant.hh"

#include <cassert>
#include <random>
#include <type_traits>

#include "domain.hh"
#include "non_noisy_relation.hh"

// TODO(emilyaf): Implement this for NoisyRelation.
RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains) {
  std::mt19937 prng;
  DistributionVariant dv = cluster_prior_from_spec(dist_spec, &prng);

  RelationVariant rv;

  // We want to go from dv to its SampleType.  This only takes five steps:
  // 1. To go from the DistributionVariant dv to the underlying
  //    Distribution pointer, we use a std::visit.
  // 2. To get the type of the Distribution, we use decltype(*v).
  // 3. But that turns out to be of type DistributionName&, so we need to use
  //    a std::remove_reference_t to get rid of the &.
  // 4. You would think that we could now just access ::SampleType and be done,
  //    but no -- that expression is sufficiently complicated that the C++
  //    parser gets confused and throws an error about "dependent-name ... is
  //    parsed as a non-type".  Luckily the same error message also gives the
  //    fix:  just add a typename to the beginning.
  // 5. With that, we can finally access the SampleType and use it to construct
  //    the right kind of Relation.
  std::visit(
      [&](const auto& v) {
        rv = new NonNoisyRelation<
            typename std::remove_reference_t<decltype(*v)>::SampleType>(
            name, dist_spec, domains);
      },
      dv);

  return rv;
}
