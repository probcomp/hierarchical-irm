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
  PCleanSchemaHelper(const PCleanSchema& s,
                     bool _only_final_emissions = false,
                     bool _record_class_is_clean = true);

  // Translate the PCleanSchema into an HIRM T_schema.
  // Also, fill annotated_domains_for_relation[r] with the vector of
  // annotated domains for the relation r.
  T_schema make_hirm_schema(
      std::map<std::string, std::vector<std::string>>
      *annotated_domains_for_relation);

  // The rest of these methods are conceptually private, but actually
  // public for testing.

  void compute_domains_cache();

  void compute_domains_for(const std::string& name);

  void make_relations_for_queryfield(
      const QueryField& f, const PCleanClass& c,
      T_schema* schema);

  PCleanSchema schema;
  bool only_final_emissions;
  bool record_class_is_clean;
  std::map<std::string, std::vector<std::string>> domains;
  std::map<std::string, std::vector<std::string>> annotated_domains;
};

// Returns original_domains, but with the elements corresponding to
// annotated_ds elements that start with prefix moved to the front.
std::vector<std::string> reorder_domains(
    const std::vector<std::string>& original_domains,
    const std::vector<std::string>& annotated_ds,
    const std::string& prefix);
