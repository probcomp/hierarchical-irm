// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "util_distribution_variant.hh"

class BetaBernoulli;
class Bigram;
class DirichletCategorical;
class Normal;
class Domain;
template <typename DistributionType>
class Relation;

using RelationVariant =
    std::variant<Relation<BetaBernoulli>*, Relation<Bigram>*,
                 Relation<DirichletCategorical>*, Relation<Normal>*>;

RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains);
