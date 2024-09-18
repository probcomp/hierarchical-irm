// Copyright 2024
// See LICENSE.txt

#pragma once
#include <map>
#include <random>
#include <string>

#include "distributions/crp.hh"
#include "hirm.hh"
#include "observations.hh"
#include "pclean/schema.hh"
#include "pclean/schema_helper.hh"

class GenDB {
 public:
  const PCleanSchema& schema;

  // TODO(emilyaf): Merge PCleanSchemaHelper and GenDB.
  PCleanSchemaHelper schema_helper;

  // This data structure contains entity sets and linkages. Semantics are
  // map<tuple<class_name, reference_field_name, class_primary_key> ref_val>>,
  // where primary_key and ref_val are (integer) entity IDs.
  std::map<std::tuple<std::string, std::string, int>, int> reference_values;

  HIRM* hirm;  // Owned by the GenDB instance.

  // Map keys are class names. Values are CRPs for latent entities, where the
  // "tables" are entity IDs and the "customers" are unique identifiers of
  // observations of that class.
  std::map<std::string, CRP> domain_crps;

  GenDB(std::mt19937* prng, const PCleanSchema& schema,
        bool _only_final_emissions = false, bool _record_class_is_clean = true);

  // Incorporates a row of observed data into the GenDB instance.
  void incorporate(
      std::mt19937* prng,
      const std::pair<int, std::map<std::string, ObservationVariant>>& row);

  // Incorporates a single element of a row of observed data.
  void incorporate_query_relation(std::mt19937* prng,
                                  const std::string& query_rel,
                                  const T_items& items,
                                  const ObservationVariant& value);

  // Samples a reference value and stores it in reference_values and the
  // relevant domain CRP.
  void sample_and_incorporate_reference(
      std::mt19937* prng,
      const std::tuple<std::string, std::string, int>& ref_key,
      const std::string& ref_class);

  // Samples a set of entities in the domains of the relation corresponding to
  // class_path.
  T_items sample_entities_relation(
      std::mt19937* prng, const std::string& class_name,
      std::vector<std::string>::const_iterator class_path_start,
      std::vector<std::string>::const_iterator class_path_end, int class_item);

  // Sample items from a class' ancestors (recursive reference fields).
  T_items sample_class_ancestors(std::mt19937* prng,
                                 const std::string& class_name, int class_item);

  ~GenDB();

  // Disable copying.
  GenDB& operator=(const GenDB&) = delete;
  GenDB(const GenDB&) = delete;
};