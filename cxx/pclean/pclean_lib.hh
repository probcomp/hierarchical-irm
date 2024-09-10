// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include "irm.hh"
#include "util_io.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema.hh"

// For each non-missing value in the DataFrame df, create an
// observation in the returned T_observations.  The column name of the value
// is used as the relation name, and each entity in each domain is given
// its own unique value.
T_observations translate_observations(
    const DataFrame& df, const T_schema &schema,
    const std::map<std::string, std::vector<std::string>>
    &annotated_domains_for_relation);

// Return a dataframe of num_samples samples from the HIRM.
DataFrame make_pclean_samples(int num_samples, const HIRM &hirm,
                              std::mt19937* prng);
