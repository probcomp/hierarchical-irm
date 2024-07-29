// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "irm.hh"

struct DistributionVar {
  std::string distribution_name;
  std::map<std::string, std::string> distribution_params;
};


struct EmissionVar {
  std::string emission_name;
  std::map<std::string, std::string> emission_params;
  // Currently only length two field_paths of the form
  // [name_of_ClassVar_in_this_class, name_of_variable_in_that_class]
  // are supported.
  std::vector<std::string> field_path;
};

struct ClassVar {
  std::string class_name;
};

struct PCleanVariable {
  std::string name;
  std::variant<DistributionVar, EmissionVar, ClassVar> spec;
};

struct PCleanClass {
  std::string name;
  std::vector<PCleanVariable> vars;
  // TODO(thomaswc): Figure out how to handle class level configurations.
};

struct QueryField {
  // "physician.school_name as School" gets parsed as
  // {name="School", class_path=["physician","school"]}.
  std::string name;
  std::vector<std::string> class_path;
};

struct PCleanQuery {
  std::string base_class;
  std::vector<QueryField> fields;
};

struct PCleanSchema {
  std::vector<PCleanClass> classes;
  PCleanQuery query;
};

// A class for quickly computing various properties of the schema.
class PCleanSchemaHelper {
 public:
  PCleanSchemaHelper(const PCleanSchema& s);

  PCleanClass get_class_by_name(const std::string& name);

  // The parent classes of a class are those that are referred to by a
  // ClassVar or EmissionVar inside the class.
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
