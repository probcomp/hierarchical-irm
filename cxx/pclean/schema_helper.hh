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

  // The parent classes of a class are those that are referred to by a
  // ClassVar inside the class.
  std::set<std::string> get_parent_classes(const std::string& name);
  // The ancestors of a class are the transitive closure of the parent
  // relationship.
  std::set<std::string> get_ancestor_classes(const std::string& name);
  // The source classes of a class are its ancestors without parents.
  std::set<std::string> get_source_classes(const std::string& name);

  T_schema make_hirm_schema();

 private:
  void compute_class_name_cache();
  void compute_ancestors_cache();
  std::set<std::string> compute_ancestors_for(const std::string& name);

  PCleanSchema schema;
  std::map<std::string, int> class_name_to_index;
  std::map<std::string, std::set<std::string>> ancestors;
};
