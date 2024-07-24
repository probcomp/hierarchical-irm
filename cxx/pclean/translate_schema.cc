// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <set>
#include <string>
#include <variant>
#include <vector>

#include "util_distribution_variant.hh"
#include "pclean/translate_schema.hh"

const PCleanClass &get_class_by_name(
    const PCleanSchema& pclean_schema,
    const std::string& class_name) {
  for (auto const& c : pclean_schema.classes) {
    if (c.name == class_name) {
      return c;
    }
  }
  return NULL;
}

// Returns the set of classes that originate paths to class_name.
std::set<std::string> get_source_classes(
    const PCleanSchema& pclean_schema,
    const PCleanClass& pclean_class) {
  std::set<std::string> sources;
  for (auto const &v : pclean_class.vars) {
    if (std::holds_alternative<ClassVar>(v.spec)) {
      std::string cname = std::get<ClassVar>(v.spec).class_name;
      if (sources.find(cname) == sources.end()) {
        continue;
      }
      sources.insert(cname);
      const PCleanClass &pcc = get_class_by_name(pclean_schema, cname);
      std::set<std::string> transitive_sources = get_source_classes(
          pclean_schema, pcc);
      sources.insert(transitive_sources.begin(), transitive_sources.end());
    }
    // TODO: Handle EmissionVar's
  }
  return sources;
}

void translate_pclean_schema_to_hirm_schema(
    const PCleanSchema& pclean_schema,
    T_schema *hirm_schema) {
  // First translate distribution variables into clean relations.
  for (auto const& c : pclean_schema.classes) {
    for (auto const& v : c.vars) {
      if (std::holds_alternative<DistributionVar>(v.spec)) {
        T_clean_relation relation;
        DistributionVar dv = std::get<DistributionVar>(v.spec);
        relation.distribution_spec = DistributionSpec(
            dv.distribution_name, dv.distribution_params);
        relation.is_observed = false;
        std::set<std::string> source_classes = get_source_classes(
            pclean_schema, c);
        for (const std::string& sc : source_classes) {
          relation.domains.push_back(sc);
        }
        (*hirm_schema)[v.name] = relation;
      }
    }
  }
}
