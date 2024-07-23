// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

struct DistributionVar {
  std::string distribution_name;
  std::map<std::string, std::string> distribution_params;
};


struct EmissionVar {
  std::string emission_name;
  std::map<std::string, std::string> emission_params;
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
