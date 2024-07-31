#include "pclean/schema_helper.hh"
#include "pclean/get_joint_relations.hh"

PCleanSchemaHelper::PCleanSchemaHelper(const PCleanSchema& s): schema(s) {
  compute_class_name_cache();
  compute_domains_cache();
}

void PCleanSchemaHelper::compute_class_name_cache() {
  for (size_t i = 0; i < schema.classes.size(); ++i) {
    class_name_to_index[schema.classes[i].name] = i;
  }
}

void PCleanSchemaHelper::compute_domains_cache() {
  for (const auto& c: schema.classes) {
    if (!domains.contains(c.name)) {
      domains[c.name] = compute_domains_for(c.name);
    }
  }
}

std::vector<std::string> PCleanSchemaHelper::compute_domains_for(
    const std::string& name) {
  std::vector<std::string> ds;
  ds.push_back(name);
  PCleanClass c = get_class_by_name(name);

  for (const auto& v: c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(v.spec))) {
      if (!domains.contains(cv->class_name)) {
        domains[cv->class_name] = compute_domains_for(cv->class_name);
      }
      for (const std::string& s : domains[cv->class_name]) {
        ds.push_back(v.name + ':' + s);
      }
    }
  }

  return ds;
}

PCleanClass PCleanSchemaHelper::get_class_by_name(const std::string& name) {
  return schema.classes[class_name_to_index[name]];
}

PCleanVariable PCleanSchemaHelper::get_scalarvar_from_path(
    const PCleanClass& base_class,
    std::vector<std::string>::const_iterator path_iterator,
    std::string* final_class_name) {
  const std::string& s = *path_iterator;
  for (const PCleanVariable& v : base_class.vars) {
    if (v.name == s) {
      if (std::holds_alternative<ScalarVar>(v.spec)) {
        *final_class_name = base_class.name;
        return v;
      }
      const PCleanClass& next_class = get_class_by_name(
          std::get<ClassVar>(v.spec).class_name);
      PCleanVariable sv = get_scalarvar_from_path(
          next_class, ++path_iterator, final_class_name);
      return sv;
    }
  }
  printf("Error: could not find name %s in class %s\n",
         s.c_str(), base_class.name.c_str());
  assert(false);
}

T_schema PCleanSchemaHelper::make_hirm_schema() {
  T_schema tschema;
  for (const auto& c : schema.classes) {
    for (const auto& v : c.vars) {
      std::string rel_name = c.name + ':' + v.name;
      if (const ScalarVar* dv = std::get_if<ScalarVar>(&(v.spec))) {
        tschema[rel_name] = get_distribution_relation(*dv, domains[c.name]);
      }
    }
  }

  const PCleanClass query_class = get_class_by_name(schema.query.base_class);
  for (const auto& f : schema.query.fields) {
    std::string final_class_name;
    const PCleanVariable sv = get_scalarvar_from_path(
        query_class, f.class_path.cbegin(), &final_class_name);
    std::string base_relation = final_class_name + ':' + sv.name;
    tschema[f.name] = get_emission_relation(
        std::get<ScalarVar>(sv.spec), domains[query_class.name], base_relation);
  }

  return tschema;
}
