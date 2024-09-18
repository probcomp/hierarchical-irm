// Copyright 2024
// See LICENSE.txt

#include "gendb.hh"

#include <map>
#include <random>
#include <string>

#include "distributions/crp.hh"
#include "hirm.hh"
#include "irm.hh"
#include "observations.hh"
#include "pclean/schema.hh"
#include "pclean/schema_helper.hh"

GenDB::GenDB(std::mt19937* prng, const PCleanSchema& schema_,
             bool _only_final_emissions, bool _record_class_is_clean)
    : schema(schema_),
      schema_helper(schema_, _only_final_emissions, _record_class_is_clean) {
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  T_schema hirm_schema =
      schema_helper.make_hirm_schema(&annotated_domains_for_relation);
  hirm = new HIRM(hirm_schema, prng);

  for (const auto& [class_name, unused_class] : schema.classes) {
    domain_crps[class_name] = CRP();
  }
}

void GenDB::incorporate(
    std::mt19937* prng,
    const std::pair<int, std::map<std::string, ObservationVariant>>& row) {
  int id = row.first;

  // Maps a query relation name to an observed value.
  std::map<std::string, ObservationVariant> vals = row.second;

  // Loop over the values in the row, and their query relation names.
  for (const auto& [query_rel, val] : vals) {
    // Sample a set of items to be incorporated into the query relation.
    const std::vector<std::string>& class_path =
        schema.query.fields.at(query_rel).class_path;
    std::vector<int> items =
        sample_entities_relation(prng, schema.query.record_class,
                                 class_path.cbegin(), class_path.cend(), id);

    // Incorporate the items/value into the query relation.
    incorporate_query_relation(prng, query_rel, items, val);
  }
}

// This function walks the class_path of the query, populates the global
// reference_values table if necessary, and returns a sampled set of items
// for the query relation.
std::vector<int> GenDB::sample_entities_relation(
    std::mt19937* prng, const std::string& class_name,
    std::vector<std::string>::const_iterator class_path_start,
    std::vector<std::string>::const_iterator class_path_end, int class_item) {
  if (class_path_end - class_path_start == 1) {
    // These are domains and we need to DFS-traverse the class's
    // parents, similar to PCleanSchemaHelper::compute_domains_for.
    return sample_class_ancestors(prng, class_name, class_item);
  } else {
    // These are noisy relation domains along the path from the latent cleanly-
    // observed class to the record class.
    std::string ref_field = *class_path_start;

    // If the reference field isn't populated, sample a value from a CRP and
    // add it to reference_values.
    std::string ref_class =
        std::get<ClassVar>(
            schema.classes.at(class_name).vars.at(ref_field).spec)
            .class_name;
    if (!reference_values[class_name].contains(class_item)) {
      sample_and_incorporate_reference(prng, class_name, class_item, ref_field,
                                       ref_class);
    }
    std::vector<int> items = sample_entities_relation(
        prng, ref_class, ++class_path_start, class_path_end,
        reference_values[class_name][class_item][ref_field]);
    items.push_back(class_item);
    return items;
  }
}

void GenDB::sample_and_incorporate_reference(std::mt19937* prng,
                                             const std::string& class_name,
                                             int class_item,
                                             const std::string& ref_field,
                                             const std::string& ref_class) {
  int new_val = domain_crps[ref_class].sample(prng);

  // Generate a unique ID for the sample and incorporate it into the
  // domain CRP.
  std::stringstream new_id_str;
  new_id_str << class_name << class_item << ref_field;
  int new_id = std::hash<std::string>{}(new_id_str.str());
  reference_values[class_name][class_item][ref_field] = new_val;
  domain_crps[ref_class].incorporate(new_id, new_val);
}

// Recursively incorporates samples into base relations.
void GenDB::incorporate_query_relation(std::mt19937* prng,
                                       const std::string& query_rel_name,
                                       const T_items& items,
                                       const ObservationVariant& value) {
  RelationVariant query_rel = hirm->get_relation(query_rel_name);
  T_items base_items = std::visit(
      [&](auto nr) {
        using T = typename std::remove_pointer_t<decltype(nr)>::ValueType;
        auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(nr);
        return noisy_rel->get_base_items(items);
      },
      query_rel);

  T_noisy_relation t_query_rel =
      std::get<T_noisy_relation>(hirm->schema.at(query_rel_name));
  bool base_contains_items = std::visit(
      [&](auto rel) { return rel->get_data().contains(base_items); },
      hirm->get_relation(t_query_rel.base_relation));
  if (!base_contains_items) {
    hirm->sample_and_incorporate_relation(prng, t_query_rel.base_relation,
                                          base_items);
  }
  hirm->incorporate(prng, query_rel_name, items, value);
}

// Generates a vector of items from the clean relation domains, with the
// primary key (final item) equal to class_item. Items are looked up in the
// global reference_values table or sampled from CRPs (and added to the
// reference_values table/entity CRPs) if necessary.
std::vector<int> GenDB::sample_class_ancestors(
    std::mt19937* prng, const std::string& class_name, int class_item) {
  T_items items;
  PCleanClass c = schema.classes.at(class_name);

  for (const auto& [name, var] : c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(var.spec))) {
      // If the reference field isn't populated, sample a value from a CRP and
      // add it to reference_values.
      if (!reference_values[class_name][class_item].contains(name)) {
        sample_and_incorporate_reference(prng, class_name, class_item, name,
                                         cv->class_name);
      }
      T_items ref_items = sample_class_ancestors(
          prng, cv->class_name, reference_values[class_name][class_item][name]);
      items.insert(items.end(), ref_items.begin(), ref_items.end());
    }
  }

  items.push_back(class_item);
  return items;
}

GenDB::~GenDB() { delete hirm; }
