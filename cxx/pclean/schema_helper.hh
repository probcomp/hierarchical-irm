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
  PCleanSchemaHelper(const PCleanSchema& s);

  PCleanClass get_class_by_name(const std::string& name);

  T_schema make_hirm_schema();

  // The rest of these methods are conceptually private, but actually
  // public for testing.

  void compute_class_name_cache();
  void compute_domains_cache();

  std::vector<std::string> compute_domains_for(const std::string& name);

  PCleanVariable get_scalarvar_from_path(
      const PCleanClass& base_class,
      std::vector<std::string>::const_iterator path_iterator,
      std::string* final_class_name);

  PCleanSchema schema;
  std::map<std::string, int> class_name_to_index;
  std::map<std::string, std::vector<std::string>> domains;
};
