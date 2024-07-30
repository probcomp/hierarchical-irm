// Copyright 2024
// See LICENSE.txt

#pragma once

#include <string>
#include <vector>

#include "clean_relation.hh"
#include "noisy_relation.hh"
#include "pclean/schema.hh"

// Given the joint_name and params in the ScalarVar, and a list of domains,
// return a clean elation containing the generative distribution for the
// variable.
T_clean_relation get_distribution_relation(
    const ScalarVar& scalar_var,
    const std::vector<std::string>& domains);

// Given the joint_name and params in the ScalarVar, and a list of domains,
// return a noisy relation containing the emission for the variable.
T_noisy_relation get_emission_relation(
    const ScalarVar& scalar_var,
    const std::vector<std::string>& domains,
    const std::string& base_relation);
