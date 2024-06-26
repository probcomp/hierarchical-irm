// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "domain.hh"
#include "relation.hh"
#include "util_distribution_variant.hh"

using RelationVariant =
    std::variant<Relation<std::string>*, Relation<double>*,
                 Relation<int>*, Relation<bool>*>;

RelationVariant relation_from_spec(const std::string& name,
                                   const DistributionSpec& dist_spec,
                                   std::vector<Domain*>& domains);
