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
  // map<class_name, map<primary_key, map<reference_field_name, ref_val>>>,
  // where primary_key and ref_val are (integer) entity IDs.
  std::map<std::string, std::map<int, std::map<std::string, int>>>
      reference_values;

  HIRM* hirm;

  // CRPs for latent entities, where the "tables" are entity IDs and the
  // "customers" are unique identifiers of observations of that class. Map
  // keys are class names.
  std::map<std::string, CRP> domain_crps;

  GenDB(std::mt19937* prng, const PCleanSchema& schema,
        bool _only_final_emissions = false, bool _record_class_is_clean = true);

  void incorporate(
      std::mt19937* prng,
      const std::pair<int, std::map<std::string, ObservationVariant>>& row);

  void incorporate_query_relation(std::mt19937* prng, 
                                  const std::string& query_rel,
                                  const T_items& items,
                                  const ObservationVariant& value);

  void sample_and_incorporate_reference(
    std::mt19937* prng, const std::string& class_name, int class_item, 
    const std::string& ref_field, const std::string& ref_class);

  std::vector<int> sample_entities_relation(
      std::mt19937* prng, const std::string& class_name,
      std::vector<std::string>::const_iterator class_path_start,
      std::vector<std::string>::const_iterator class_path_end,
      int class_item);

  std::vector<int> sample_class_ancestors(
      std::mt19937* prng, const std::string& class_name, int class_item);

  ~GenDB();

  // Disable copying.
  GenDB& operator=(const GenDB&) = delete;
  GenDB(const GenDB&) = delete;
};