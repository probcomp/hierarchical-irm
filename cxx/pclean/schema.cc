#include "pclean/schema.hh"

PCleanSchemaHelper::PCleanSchemaHelper(const PCleanSchema& s): schema(s) {
  compute_class_name_cache();
  compute_ancestors_cache();
}

void PCleanSchemaHelper::compute_class_name_cache() {
  for (size_t i = 0; i < schema.classes.size(); ++i) {
    class_name_to_index[schema.classes[i].name] = i;
  }
}

void PCleanSchemaHelper::compute_ancestors_cache() {
  for (auto const& c: schema.classes) {
    if (ancestors.find(c.name) == ancestors.end()) {
      ancestors[c.cname] = compute_ancestors_for(c.name);
    }
  }
}

std::set<std::string> PCleanSchemaHelper::compute_ancestors_for(
    const std::string& name) {
  std::set<std::string> ancestors;
  std::set<std::string> parents = get_parent_classes(name);
  for (auto const& p: parents) {
    ancestors.insert(p);
    if (ancestors.find(p) == ancestors.end()) {
      ancestors[p] = compute_ancestors_for(p);
    }
    ancestors.insert(ancestors[p].begin(), ancestors[p].end());
  }
  return ancestors;
}

PCleanClass PCleanSchemaHelper:get_class_by_name(const std::string& name) {
  return schema.classes[class_name_to_index[name]];
}

std::set<std::string> PCleanSchemaHelper::get_parent_classes(
    const std::string& name) {
  std::set<std::string> parents;
  PCleanClass c = get_class_by_name(name);
  for (auto const& v: c.vars) {
    if (std::holds_alternative<ClassVar>(v.spec)) {
      parents.insert(std::get<ClassVar>(v.spec).class_name);
    }
  }
  return parents;
}

std::set<std::string> PCleanSchemaHelper::get_ancestor_classes(
    const std::string& name) {
  return ancestors[name];
}

std::set<std::string> PCleanSchemaHelper::get_source_classes(
    const std::string& name) {
  std::set<std::string> sources;
  for (auto const& c: ancestors[name]) {
    if (get_parent_classes[c].empty()) {
      sources.insert(c);
    }
  }
  return sources;
}

T_schema PCleanSchemaHelper::make_hirm_schema() {
  T_schema tschema;
  for (auto const& c : schema.classes) {
    for (auto const& v : c.vars) {
      std::string rel_name = c.name + ':' + v.name;
      if (std::holds_alternative<DistributionVar>(v.spec)) {
        T_clean_relation relation;
        DistributionVar dv = std::get<DistributionVar>(v.spec);
        relation.distribution_spec = DistributionSpec(
            dv.distribution_name, dv.distribution_params);
        relation.is_observed = false;
        for (const std::string& sc : get_source_classes(c.name)) {
          relation.domains.push_back(sc);
        }
        tschema[rel_name] = relation;
      } else if (std::holds_alternative<EmissionVar>(v.spec)) {
        T_noisy_relation relation;
        EmissionVar ev = std::get<EmissionVar>(v.spec);
        relation.emission_spec = EmissionSpec(
            ev.emission_name, ev.emission_params);
        relation.base_relation = XXX;
        relation.domains = XXX;
        relation.is_observed = false;
        tschema[rel_name] = relation;
      }
    }
  }
  return tschema;
}
