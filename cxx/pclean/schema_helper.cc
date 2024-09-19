#include <cstdlib>

#include "pclean/schema_helper.hh"
#include "pclean/get_joint_relations.hh"

PCleanSchemaHelper::PCleanSchemaHelper(
    const PCleanSchema& s,
    bool _only_final_emissions,
    bool _record_class_is_clean):
  schema(s), only_final_emissions(_only_final_emissions),
  record_class_is_clean(_record_class_is_clean) {
  compute_domains_cache();
}

void PCleanSchemaHelper::compute_domains_cache() {
  for (const auto& c: schema.classes) {
    if (!domains.contains(c.first)) {
      compute_domains_for(c.first);
    }
  }
}

void PCleanSchemaHelper::compute_domains_for(const std::string& name) {
  std::vector<std::string> ds;
  std::vector<std::string> annotated_ds;
  PCleanClass c = schema.classes[name];

  for (const auto& v: c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(v.second.spec))) {
      if (!domains.contains(cv->class_name)) {
        compute_domains_for(cv->class_name);
      }
      for (const std::string& s : domains[cv->class_name]) {
        ds.push_back(s);
      }
      for (const std::string& s : annotated_domains[cv->class_name]) {
        annotated_ds.push_back(v.first + ':' + s);
      }
    }
  }

  // Put the "primary" domain last, so that it survives reordering.
  ds.push_back(name);
  annotated_ds.push_back(name);

  domains[name] = ds;
  annotated_domains[name] = annotated_ds;
}

void PCleanSchemaHelper::make_relations_for_queryfield(
    const QueryField& f, const PCleanClass& record_class, T_schema* tschema,
    std::map<std::string, std::vector<std::string>>
    *annotated_domains_for_relation) {
  // First, find all the vars and classes specified in f.class_path.
  std::vector<std::string> var_names;
  std::vector<std::string> class_names;
  PCleanVariable last_var;
  PCleanClass last_class = record_class;
  class_names.push_back(record_class.name);
  for (size_t i = 0; i < f.class_path.size(); ++i) {
    const PCleanVariable& v = last_class.vars[f.class_path[i]];
    last_var = v;
    var_names.push_back(v.name);
    if (i < f.class_path.size() - 1) {
      class_names.push_back(std::get<ClassVar>(v.spec).class_name);
      last_class = schema.classes[class_names.back()];
    }
  }
  // Remove the last var_name because it isn't used in making the path_prefix.
  var_names.pop_back();

  // Get the base relation from the last class and variable name.
  std::string base_relation_name = class_names.back() + ":" + last_var.name;

  // Handle queries of the record class specially.
  if (f.class_path.size() == 1) {
    if (record_class_is_clean) {
      // Just rename the existing clean relation and set it to be observed.
      T_clean_relation cr = std::get<T_clean_relation>(
          tschema->at(base_relation_name));
      cr.is_observed = true;
      (*tschema)[f.name] = cr;
      tschema->erase(base_relation_name);
      (*annotated_domains_for_relation)[f.name] = annotated_domains[
          record_class.name];
    } else {
      T_noisy_relation tnr = get_emission_relation(
          std::get<ScalarVar>(last_var.spec),
          domains[record_class.name],
          base_relation_name);
      tnr.is_observed = true;
      (*tschema)[f.name] = tnr;
      (*annotated_domains_for_relation)[f.name] = annotated_domains[
          record_class.name];
    }
    return;
  }

  // Handle only_final_emissions == true.
  if (only_final_emissions) {
    std::vector<std::string> noisy_domains = domains[class_names.back()];
    std::vector<std::string> adfr = annotated_domains[class_names.back()];
    for (int i = class_names.size() - 2; i >= 0; --i) {
      noisy_domains.push_back(class_names[i]);
      for (size_t j = 0; j < adfr.size(); ++j) {
        adfr[j] = var_names[i] + ":" + adfr[j];
      }
      adfr.push_back(class_names[i]);
    }
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(last_var.spec),
        noisy_domains,
        base_relation_name);
    tnr.is_observed = true;
    (*tschema)[f.name] = tnr;
    (*annotated_domains_for_relation)[f.name] = adfr;
    return;
  }

  // Handle only_final_emissions == false.
  std::string& previous_relation = base_relation_name;
  std::vector<std::string> current_domains = domains[class_names.back()];
  std::vector<std::string> adfr = annotated_domains[class_names.back()];
  for (int i = f.class_path.size() - 2; i >= 0; --i) {
    current_domains.push_back(class_names[i]);
    for (size_t j = 0; j < adfr.size(); ++j) {
      adfr[j] = var_names[i] + ":" + adfr[j];
    }
    adfr.push_back(class_names[i]);
    T_noisy_relation tnr = get_emission_relation(
        std::get<ScalarVar>(last_var.spec),
        current_domains,
        previous_relation);
    std::string rel_name;
    if (i == 0) {
      rel_name = f.name;
      tnr.is_observed = true;
    } else {
      // Intermediate emissions have a name of the form
      // "[Observing Class]::[QueryFieldName]"
      rel_name = class_names[i] + "::" + f.name;
      tnr.is_observed = false;
    }
    (*tschema)[rel_name] = tnr;
    previous_relation = rel_name;
    (*annotated_domains_for_relation)[rel_name] = adfr;
  }
}

T_schema PCleanSchemaHelper::make_hirm_schema(
    std::map<std::string, std::vector<std::string>>
    *annotated_domains_for_relation) {
  T_schema tschema;

  // For every scalar variable, make a clean relation with the name
  // "[ClassName]:[VariableName]".
  for (const auto& c : schema.classes) {
    for (const auto& v : c.second.vars) {
      std::string rel_name = c.first + ':' + v.first;
      if (const ScalarVar* dv = std::get_if<ScalarVar>(&(v.second.spec))) {
        tschema[rel_name] = get_distribution_relation(*dv, domains[c.first]);
        (*annotated_domains_for_relation)[rel_name] = annotated_domains[c.first];
      }
    }
  }

  // For every query field, make one or more relations by walking up
  // the class_path.  At least one of those relations will have name equal
  // to the name of the QueryField.
  const PCleanClass record_class = schema.classes[schema.query.record_class];
  for (const auto& [unused_name, f] : schema.query.fields) {
    make_relations_for_queryfield(f, record_class, &tschema,
                                  annotated_domains_for_relation);
  }

  return tschema;
}
