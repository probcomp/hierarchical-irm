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
      compute_domains_for(c.name);
    }
  }
}

void PCleanSchemaHelper::compute_domains_for(const std::string& name) {
  std::vector<std::string> ds;
  std::vector<std::string> annotated_ds;
  ds.push_back(name);
  annotated_ds.push_back(name);
  PCleanClass c = get_class_by_name(name);

  for (const auto& v: c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(v.spec))) {
      if (!domains.contains(cv->class_name)) {
        compute_domains_for(cv->class_name);
      }
      for (const std::string& s : domains[cv->class_name]) {
        ds.push_back(s);
      }
      for (const std::string& s : annotated_domains[cv->class_name]) {
        annotated_ds.push_back(v.name + ':' + s);
      }
    }
  }

  domains[name] = ds;
  annotated_domains[name] = annotated_ds;
}

PCleanClass PCleanSchemaHelper::get_class_by_name(const std::string& name) {
  return schema.classes[class_name_to_index[name]];
}

PCleanVariable PCleanSchemaHelper::get_scalarvar_from_path(
    const PCleanClass& base_class,
    std::vector<std::string>::const_iterator path_iterator,
    std::string* final_class_name,
    std::string* path_prefix) {
  const std::string& s = *path_iterator;
  for (const PCleanVariable& v : base_class.vars) {
    if (v.name == s) {
      if (std::holds_alternative<ScalarVar>(v.spec)) {
        *final_class_name = base_class.name;
        return v;
      }
      path_prefix->append(v.name + ":");
      const PCleanClass& next_class = get_class_by_name(
          std::get<ClassVar>(v.spec).class_name);
      PCleanVariable sv = get_scalarvar_from_path(
          next_class, ++path_iterator, final_class_name, path_prefix);
      return sv;
    }
  }
  printf("Error: could not find name %s in class %s\n",
         s.c_str(), base_class.name.c_str());
  assert(false);
}

// Returns original_domains, but with the elements corresponding to
// annotated_ds elements that start with prefix moved to the front.
std::vector<std::string> reorder_domains(
    const std::vector<std::string>& original_domains,
    const std::vector<std::string>& annotated_ds,
    const std::string& prefix) {
  std::vector<std::string> output_domains;
  for (size_t i = 0; i < original_domains.size(); ++i) {
    if (annotated_ds[i].starts_with(prefix)) {
      output_domains.push_back(original_domains[i]);
    }
  }
  for (size_t i = 0; i < original_domains.size(); ++i) {
    if (!annotated_ds[i].starts_with(prefix)) {
      output_domains.push_back(original_domains[i]);
    }
  }
  return output_domains;
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

  const PCleanClass query_class = get_class_by_name(schema.query.record_class);
  for (const auto& f : schema.query.fields) {
    std::string final_class_name;
    std::string path_prefix;
    const PCleanVariable sv = get_scalarvar_from_path(
        query_class, f.class_path.cbegin(), &final_class_name, &path_prefix);
    std::string base_relation = final_class_name + ':' + sv.name;
    std::vector<std::string> reordered_domains = reorder_domains(
        domains[query_class.name],
        annotated_domains[query_class.name],
        path_prefix);
    tschema[f.name] = get_emission_relation(
        std::get<ScalarVar>(sv.spec), reordered_domains, base_relation);
  }

  return tschema;
}
