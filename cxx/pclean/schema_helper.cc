#include <cstdlib>

#include "pclean/schema_helper.hh"
#include "pclean/get_joint_relations.hh"

PCleanSchemaHelper::PCleanSchemaHelper(
    const PCleanSchema& s, bool _only_final_emissions):
  schema(s), only_final_emissions(_only_final_emissions),
  query_class_is_clean(_query_class_is_clean) {
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

  // Put the "primary" domain last, so that it survives reordering.
  ds.push_back(name);
  annotated_ds.push_back(name);

  domains[name] = ds;
  annotated_domains[name] = annotated_ds;
}

PCleanClass PCleanSchemaHelper::get_class_by_name(const std::string& name) {
  return schema.classes[class_name_to_index[name]];
}

PCleanVariable find_variable_in_class(
    std::string& var_name, const PCleanClass& c) {
  for (const PCleanVariable& v: c.vars) {
    if (v.name == var_name) {
      return v;
    }
  }
  printf("Error: could not find a variable named %s in class %s!\n",
         var_name.c_str(), c.c_str());
  std::exit(1);
}

void make_relations_for_queryfield(
    const QueryField& f, const PCleanClass& query_class, T_schema* tschema) {
  // First, find all the vars and classes specified in f.class_path.
  std::vector<PCleanVariable> vars;
  std::vector<PCleanClass&> classes;
  classes.push_back(query_class);
  for (size_t i = 0; i < f.class_path.size(); ++i) {
    const PCleanVariable& v = find_variable_in_class(
        f.class_path[i], classes.back());
    vars.push_back(v);
    if (i < f.class_path.size() - 1) {
      classes.push_back(get_class_by_name(
          std::get<ClassVar>(v.spec).class_name));
    }
  }

  // Get the base relation from the last class and variable name.
  std::string base_relation_name = classes.back().name + ":" + vars.back().name;

  // Handle queries of the record class specially.
  if (class_path.size() == 1) {
    if (query_class_is_clean) {
      // Just rename the existing clean relation and set it to be observed.
      T_clean_relation cr = tschema->at(base_relation_name);
      cr.is_observed = true;
      (*tschema)[f.name] = cr;
      tschema->erase(base_relation_name);
    } else {
      T_noisy_relation tnr = get_emission_relation(
          std::get<ScalarVar>(vars.back().spec),
          domains[query_class.name],
          base_relation_name);
      tnr.is_observed = true;
      (*tschema)[f.name] = tnr;
    }
    return;
  }

  // Handle only_final_emissions == true.
  if (only_final_emissions) {
    // If the base relation has n domains, we need the first n domains
    // of this emission relation to be exactly the same (including order).
    // The base relation's annotated_domains are exactly those that start
    // with the path_prefix constructed above, and we use the fact that the
    // domains and annotated_domains are in one-to-one correspondence to
    // move the base relation's domains to the front.
    std::string path_prefix = "XXX";
    std::vector<std::string> reordered_domains = reorder_domains(
          domains[query_class.name],
          annotated_domains[query_class.name],
          path_prefix);
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(vars.back().spec),
        reordered_domains,
        base_relation_name);
    tnr.is_observed = true;
    (*tschema)[f.name] = tnr;
    return;
  }

  // Handle only_final_emissions == false.
  std::string& previous_relation = base_relation_name;
  for (size_t i = f.class_path.size() - 2; i >= 0; --i) {
    std::string path_prefix = "XXX";
    std::vector<std::string> reordered_domains = reorder_domains(
          domains[classes[i].name],
          annotated_domains[classes[i].name],
          path_prefix);
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(vars.back().spec),
        reordered_domains,
        previous_relation);
    std::string rel_name;
    if (i == 0) {
      rel_name = f.name;
      tnr.is_observed = true;
    } else {
      // Intermediate emissions have a name of the form
      // "[Observing Class]:[Observing Variable Name]::[Base Relation Name]"
      rel_name = classes[i].name + ":" + f.class_path[i] + "::" + base_relation_name;
      tnr.is_observed = false;
    }
    if (!tschema->contains(rel_name)) {
      (*tschema)[rel_name] = tnr;
    }
    previous_relation = rel_name;
  }
}

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

  // For every scalar variable, make a clean relation with the name
  // "[ClassName]:[VariableName]".
  for (const auto& c : schema.classes) {
    for (const auto& v : c.vars) {
      std::string rel_name = c.name + ':' + v.name;
      if (const ScalarVar* dv = std::get_if<ScalarVar>(&(v.spec))) {
        tschema[rel_name] = get_distribution_relation(*dv, domains[c.name]);
      }
    }
  }

  // For every query field, make one or more relations by walking up
  // the class_path.  At least one of those relations will have name equal
  // to the name of the QueryField.
  const PCleanClass query_class = get_class_by_name(schema.query.record_class);
  for (const QueryField& f : schema.query.fields) {
    make_relations_for_queryfield(f, query_class, &tschema);
  }

  return tschema;
}
