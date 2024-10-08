// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include "gendb.hh"
#include "irm.hh"
#include "util_io.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema.hh"

// For each non-missing value in the DataFrame df, create an
// observation and incorporate it into the GenDB.  The column name of the value
// is used as the relation name, and each entity in each domain is given
// its own unique value.
void incorporate_observations(std::mt19937* prng,
                              GenDB *gendb,
                              const DataFrame& df);

// Return a dataframe of num_samples samples from the GenDB.
// All existing rows added to gendb should have ids < start_row.
DataFrame make_pclean_samples(int num_samples, int start_row, GenDB *gendb,
                              std::mt19937* prng);

// Makes an encoding from a GenDB.  The i-th entity from domain "domain"
// is given the name "domain:i".
T_encoding make_dummy_encoding_from_gendb(const GenDB& gendb);
