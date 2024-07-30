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
  for (const auto& c: schema.classes) {
    if (!ancestors.contains(c.name)) {
      ancestors[c.name] = compute_ancestors_for(c.name);
    }
  }
}

std::set<std::string> PCleanSchemaHelper::compute_ancestors_for(
    const std::string& name) {
  std::set<std::string> ancs;
  std::set<std::string> parents = get_parent_classes(name);
  for (const std::string& p: parents) {
    ancs.insert(p);
    if (!ancestors.contains(p)) {
      ancestors[p] = compute_ancestors_for(p);
    }
    ancs.insert(ancestors[p].cbegin(), ancestors[p].cend());
  }
  return ancs;
}

PCleanClass PCleanSchemaHelper::get_class_by_name(const std::string& name) {
  return schema.classes[class_name_to_index[name]];
}

std::set<std::string> PCleanSchemaHelper::get_parent_classes(
    const std::string& name) {
  std::set<std::string> parents;
  PCleanClass c = get_class_by_name(name);
  for (const auto& v: c.vars) {
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
  for (const std::string& c: ancestors[name]) {
    const std::set<std::string>& parents = get_parent_classes(c);
    if (parents.empty()) {
      sources.insert(c);
    }
  }
  return sources;
}

std::string get_base_relation_name(
    const PCleanClass& c, std::vector<std::string>& field_path) {
  assert(field_path.size() == 2);
  std::string& class_var = field_path[0];
  std::string& var_name = field_path[1];
  std::string class_name = "";
  for (const auto& v : c.vars) {
    if (v.name == class_var) {
      class_name = std::get<ClassVar>(v.spec).class_name;
      break;
    }
  }
  assert(class_name != "");
  return class_name + ':' + var_name;
}

T_schema PCleanSchemaHelper::make_hirm_schema() {
  T_schema tschema;
  for (const auto& c : schema.classes) {
    for (const auto& v : c.vars) {
      std::string rel_name = c.name + ':' + v.name;
      if (std::holds_alternative<DistributionVar>(v.spec)) {
        T_clean_relation relation;
        DistributionVar dv = std::get<DistributionVar>(v.spec);
        relation.distribution_spec = DistributionSpec(
            dv.distribution_name, dv.distribution_params);
        relation.is_observed = false;
        relation.domains.push_back(c.name);
        for (const std::string& sc : get_source_classes(c.name)) {
          relation.domains.push_back(sc);
        }
        tschema[rel_name] = relation;
      } else if (std::holds_alternative<EmissionVar>(v.spec)) {
        T_noisy_relation relation;
        EmissionVar ev = std::get<EmissionVar>(v.spec);
        relation.emission_spec = EmissionSpec(
            ev.emission_name, ev.emission_params);
        relation.base_relation = get_base_relation_name(c, ev.field_path);
        relation.is_observed = false;
        relation.domains.push_back(c.name);
        for (const std::string& sc : get_source_classes(c.name)) {
          relation.domains.push_back(sc);
        }
        tschema[rel_name] = relation;
      }
      // TODO(thomaswc): If this class isn't the observation class,
      // create additional noisy relations.
    }
  }
  return tschema;
}
