// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <set>
#include <string>

#include "irm.hh"
#include "pclean/schema.hh"

// A class for quickly computing various properties of the schema.
class PCleanSchemaHelper {
 public:
  PCleanSchemaHelper(const PCleanSchema& s, bool _only_final_emissions = false,
                     bool _record_class_is_clean = true);

  // Translate the PCleanSchema into an HIRM T_schema.
  // Also, fill annotated_domains_for_relation[r] with the vector of
  // annotated domains for the relation r.
  T_schema make_hirm_schema(std::map<std::string, std::vector<std::string>>*
                                annotated_domains_for_relation);

  // The rest of these methods are conceptually private, but actually
  // public for testing.

  void compute_domains_cache();

  void compute_domains_for(const std::string& name);

  void make_relations_for_queryfield(
      const QueryField& f, const PCleanClass& c, T_schema* schema,
      std::map<std::string, std::vector<std::string>>*
          annotated_domains_for_relation);

  PCleanSchema schema;
  bool only_final_emissions;
  bool record_class_is_clean;
  std::map<std::string, std::vector<std::string>> domains;
  std::map<std::string, std::vector<std::string>> annotated_domains;

  // Map keys are relation name, item index of a class, and reference field
  // name. The values in the inner map are the item index of the reference
  // class. (See tests for more intuition.)
  std::map<std::string, std::map<int, std::map<std::string, int>>>
      relation_reference_indices;

  // Map keys are class name, item index of a class, and reference field
  // name. The values in the inner map are the item index of the reference
  // class. (See tests for more intuition.)
  std::map<std::string, std::map<int, std::map<std::string, int>>>
      class_reference_indices;
};
