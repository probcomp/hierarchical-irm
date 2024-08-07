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
  PCleanSchemaHelper(const PCleanSchema& s, bool _only_final_emissions = false);

  PCleanClass get_class_by_name(const std::string& name);

  T_schema make_hirm_schema();

  // The rest of these methods are conceptually private, but actually
  // public for testing.

  void compute_class_name_cache();
  void compute_domains_cache();

  void compute_domains_for(const std::string& name);

  PCleanVariable get_scalarvar_from_path(
      const PCleanClass& base_class,
      std::vector<std::string>::const_iterator path_iterator,
      std::string* final_class_name,
      std::string* path_prefix);

  PCleanSchema schema;
  bool only_final_emissions;
  std::map<std::string, int> class_name_to_index;
  std::map<std::string, std::vector<std::string>> domains;
  std::map<std::string, std::vector<std::string>> annotated_domains;
};

// Returns original_domains, but with the elements corresponding to
// annotated_ds elements that start with prefix moved to the front.
std::vector<std::string> reorder_domains(
    const std::vector<std::string>& original_domains,
    const std::vector<std::string>& annotated_ds,
    const std::string& prefix);
