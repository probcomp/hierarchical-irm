// Copyright 2024
// See LICENSE.txt

#include "gendb.hh"

#include <map>
#include <random>
#include <string>
#include <variant>
#include <iostream>

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
    reference_values[class_name];
  }
  for (const auto& [rel_name, trel] : hirm->schema) {
    const std::vector<std::string>& domains =
        std::visit([&](auto tr) { return tr.domains; }, trel);
    class_to_relations[domains.back()].push_back(rel_name);
  }
}

double GenDB::logp_score() const {
  double domain_crps_logp = 0;
  for (const auto& [d, crp] : domain_crps) {
    domain_crps_logp += crp.logp_score();
  }
  return domain_crps_logp + hirm->logp_score();
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
    T_items items =
        sample_entities_relation(prng, schema.query.record_class,
                                 class_path.cbegin(), class_path.cend(), id);

    // Incorporate the items/value into the query relation.
    incorporate_query_relation(prng, query_rel, items, val);
  }
}

// This function walks the class_path of the query, populates the global
// reference_values table if necessary, and returns a sampled set of items
// for the query relation that corresponds to the class path. class_path_start
// is an attribute of the Class named class_name.
T_items GenDB::sample_entities_relation(
    std::mt19937* prng, const std::string& class_name,
    std::vector<std::string>::const_iterator class_path_start,
    std::vector<std::string>::const_iterator class_path_end, int class_item) {
  if (class_path_end - class_path_start == 1) {
    // The last item in class_path is the class from which the queried attribute
    // is observed (for which there's a corresponding clean relation, observing
    // the attribute from the class). We need to DFS-traverse the class's
    // parents, similar to PCleanSchemaHelper::compute_domains_for.
    return sample_class_ancestors(prng, class_name, class_item);
  }

  // These are noisy relation domains along the path from the latent cleanly-
  // observed class to the record class.
  const std::string& ref_field = *class_path_start;

  // If the reference field isn't populated, sample a value from a CRP and
  // add it to reference_values.
  const std::string& ref_class =
      std::get<ClassVar>(schema.classes.at(class_name).vars.at(ref_field).spec)
          .class_name;
  std::pair<std::string, int> ref_key = {ref_field, class_item};
  if (!reference_values.at(class_name).contains(ref_key)) {
    sample_and_incorporate_reference(prng, class_name, ref_key, ref_class);
  }
  T_items items = sample_entities_relation(
      prng, ref_class, ++class_path_start, class_path_end,
      reference_values.at(class_name).at(ref_key));
  // The order of the items corresponds to the order of the relation's domains,
  // with the class (domain) corresponding to the primary key placed last on the
  // list.
  items.push_back(class_item);
  return items;
}

int GenDB::get_reference_id(const std::string& class_name,
                            const std::string& ref_field,
                            const int class_item) {
  std::stringstream new_id_str;
  std::string sep = " ";  // Spaces are disallowed in class/variable names.
  new_id_str << class_name << sep << class_item << sep << ref_field;
  return std::hash<std::string>{}(new_id_str.str());
}

void GenDB::sample_and_incorporate_reference(
    std::mt19937* prng, const std::string& class_name,
    const std::pair<std::string, int>& ref_key, const std::string& ref_class) {
  auto [ref_field, class_item] = ref_key;
  int new_val = domain_crps[ref_class].sample(prng);

  // Generate a unique ID for the sample and incorporate it into the
  // domain CRP.
  int new_id = get_reference_id(class_name, ref_field, class_item);
  reference_values.at(class_name)[ref_key] = new_val;
  domain_crps[ref_class].incorporate(new_id, new_val);
}

// Recursively incorporates samples into base relations.
void GenDB::incorporate_query_relation(std::mt19937* prng,
                                       const std::string& query_rel_name,
                                       const T_items& items,
                                       const ObservationVariant& value) {
  if (const T_noisy_relation* t_query_rel =
          std::get_if<T_noisy_relation>(&hirm->schema.at(query_rel_name))) {
    RelationVariant query_rel = hirm->get_relation(query_rel_name);
    T_items base_items = std::visit(
        [&](auto nr) {
          using T = typename std::remove_pointer_t<decltype(nr)>::ValueType;
          auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(nr);
          return noisy_rel->get_base_items(items);
        },
        query_rel);

    const std::vector<std::string>& domains =
        std::visit([&](auto tr) { return tr.domains; },
                   hirm->schema.at(t_query_rel->base_relation));
    bool base_contains_items = std::visit(
        [&](auto rel) { return rel->get_data().contains(base_items); },
        hirm->get_relation(t_query_rel->base_relation));
    if (!base_contains_items) {
      sample_and_incorporate_for_class(prng, domains.back(), base_items);
    }
  }
  hirm->incorporate(prng, query_rel_name, items, value);
}

void GenDB::sample_and_incorporate_for_class(std::mt19937* prng,
                                             const std::string& class_name,
                                             const T_items& items) {
  for (const std::string& rel_name : class_to_relations.at(class_name)) {
    if (const T_noisy_relation* t_rel =
            std::get_if<T_noisy_relation>(&hirm->schema.at(rel_name))) {
      RelationVariant rel = hirm->get_relation(rel_name);
      T_items base_items = std::visit(
          [&](auto nr) {
            using T = typename std::remove_pointer_t<decltype(nr)>::ValueType;
            auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(nr);
            return noisy_rel->get_base_items(items);
          },
          rel);
      auto t_base_rel = hirm->schema.at(t_rel->base_relation);
      auto domains = std::visit([&](auto tbf) {return tbf.domains;}, t_base_rel);
      sample_and_incorporate_for_class(prng, domains.back(), base_items);
    }
    sample_class_ancestors(prng, class_name, items.back());
    const std::vector<std::string>& domains = std::visit(
        [&](auto tr) { return tr.domains; }, hirm->schema.at(rel_name));
    T_items rel_items(domains.size());
    get_relation_items(rel_name, domains.size() - 1, items.back(), rel_items);
    bool contains_items =
        std::visit([&](auto rel) { return rel->get_data().contains(rel_items); },
                   hirm->get_relation(rel_name));
    if (!contains_items) {
      hirm->sample_and_incorporate_relation(prng, rel_name, rel_items);
    }
  }
}

// Generates a vector of items from the class' ancestors, with the
// primary key (final item) equal to class_item. Items are looked up in the
// global reference_values table or sampled from CRPs (and added to the
// reference_values table/entity CRPs) if necessary.
T_items GenDB::sample_class_ancestors(std::mt19937* prng,
                                      const std::string& class_name,
                                      int class_item) {
  T_items items;
  PCleanClass c = schema.classes.at(class_name);

  for (const auto& [name, var] : c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(var.spec))) {
      // If the reference field isn't populated, sample a value from a CRP and
      // add it to reference_values.
      std::pair<std::string, int> ref_key = {name, class_item};
      if (!reference_values.at(class_name).contains(ref_key)) {
        assert(prng != nullptr);
        sample_and_incorporate_reference(prng, class_name, ref_key,
                                         cv->class_name);
      }
      T_items ref_items = sample_class_ancestors(
          prng, cv->class_name, reference_values.at(class_name).at(ref_key));
      items.insert(items.end(), ref_items.begin(), ref_items.end());
    }
  }

  items.push_back(class_item);
  return items;
}

// This function recursively walks the tree of reference indices to populate the
// "items" vector. The top-level call should pass ind = items.size() - 1 and
// class_item equal to a primary key value (which goes in the last element of
// items and determines the rest of the values).
void GenDB::get_relation_items(const std::string& rel_name, const int ind,
                               const int class_item, T_items& items) const {
  const std::vector<std::string>& domains = std::visit(
      [&](auto tr) { return tr.domains; }, hirm->schema.at(rel_name));
  items[ind] = class_item;
  auto& ref_indices = schema_helper.relation_reference_indices;
  if (ref_indices.contains(rel_name)) {
    if (ref_indices.at(rel_name).contains(ind)) {
      for (const auto& [rf_name, rf_ind] : ref_indices.at(rel_name).at(ind)) {
        int rf_item =
            reference_values.at(domains.at(ind)).at({rf_name, class_item});
        get_relation_items(rel_name, rf_ind, rf_item, items);
      }
    }
  }
}

std::map<std::string, std::vector<size_t>> GenDB::get_domain_inds(
    const std::string& class_name, const std::string& ref_field) {
  std::map<std::string, std::vector<size_t>> domain_inds;
  for (const auto& [rel_name, query_field] : schema.query.fields) {
    // Find relations that involve both the class and its reference field
    // (exclude relations that include the class only because it's a node in
    // a noisy observation path of something otherwise unrelated to the
    // observed attribute.)
    std::vector<std::string> domains = std::visit(
        [&](auto& trel) { return trel.domains; }, hirm->schema.at(rel_name));
    for (size_t i = 0; i < domains.size(); ++i) {
      if (domains[i] == class_name &&
          schema_helper.relation_reference_indices.at(rel_name).at(i).contains(
              ref_field)) {
        domain_inds[rel_name].push_back(i);
      }
    }
  }
  return domain_inds;
}

// Unincorporates the value of class_name.ref_field where the primary key
// equals class_item.
double GenDB::unincorporate_reference(
    const std::map<std::string, std::vector<size_t>> domain_inds,
    const std::string& class_name, const std::string& ref_field,
    const int class_item,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_value_map,
    std::map<std::tuple<int, std::string, T_item>, int>&
        unincorporated_from_domains) {
  double logp_relations = 0.;
  for (auto [rel_name, inds] : domain_inds) {
    // Access the relation's items where the domain is class_name and the entity
    // value is class_item.
    RelationVariant r = hirm->get_relation(rel_name);
    bool q = std::visit(
        [&](auto rel) {
          return rel->get_data_r().at(class_name).contains(class_item);
        },
        r);
    if (q) {
      const std::unordered_set<T_items, H_items>& items = std::visit(
          [&](auto rel) {
            return rel->get_data_r().at(class_name).at(class_item);
          },
          r);
      for (const size_t ind : inds) {
        for (const auto& item : items) {
          if (item[ind] == class_item) {
            auto f = [&](auto rel) {
              logp_relations += unincorporate_reference_relation(
                  rel, rel_name, item, ind, stored_value_map);
            };
            std::visit(f, r);
          }
        }
      }
    }
  }

  // Check if any entities need to be removed from IRM domain clusters (after
  // they've been unincorporated from relations) and compute the change in logp.
  double logp_domain_cluster = 0.;
  int ref_val = reference_values.at(class_name).at({ref_field, class_item});
  for (auto& [rel_name, inds] : domain_inds) {
    for (int d_ind : inds) {
      int r_ind =
          schema_helper.relation_reference_indices.at(rel_name).at(d_ind).at(
              ref_field);
      logp_domain_cluster += unincorporate_from_domain_cluster_relation(
          rel_name, ref_val, r_ind, unincorporated_from_domains);
    }
  }
  return logp_relations + logp_domain_cluster;
}

// Recursively unincorporates from base relations and stores values.
template <typename T>
double GenDB::unincorporate_reference_relation(
    Relation<T>* rel, const std::string& rel_name, const T_items& items,
    const size_t ind,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_value_map) {
  // We can stop unincorporating from base relations if the index of
  // the domain of interest is greater than the number of domains.
  if (!rel->get_data().contains(items) || ind >= items.size()) {
    return 0.;
  }

  // Store the items/value and unincorporate them.
  T value = rel->get_value(items);
  stored_value_map[rel_name][items] = value;

  rel->unincorporate_from_cluster(items);
  std::mt19937* prng = nullptr;  // unused
  double logp_rel = rel->cluster_or_prior_logp_from_items(prng, items, value);
  // Clean up data but leave empty clusters in case they are re-populated later.
  rel->cleanup_data(items);

  // Recursively unincorporate from base relations.
  if (const T_noisy_relation* t_rel =
          std::get_if<T_noisy_relation>(&hirm->schema.at(rel_name))) {
    auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(rel);
    T_items base_items = noisy_rel->get_base_items(items);
    RelationVariant base_rel = hirm->get_relation(t_rel->base_relation);
    Relation<T>* base_rel_t = std::get<Relation<T>*>(base_rel);
    logp_rel += unincorporate_reference_relation(
        base_rel_t, t_rel->base_relation, base_items, ind, stored_value_map);
  }
  return logp_rel;
}

std::map<std::string, std::unordered_map<T_items, ObservationVariant, H_items>>
GenDB::update_reference_items(
    std::map<std::string, std::unordered_map<T_items, ObservationVariant,
                                             H_items>>& stored_values,
    const std::string& class_name, const std::string& ref_field,
    const int class_item, const int new_ref_val) {
  int old_ref_val = reference_values.at(class_name).at({ref_field, class_item});

  // Temporarily associate class_name.ref_field at index class_item with the new
  // value.
  reference_values.at(class_name)[{ref_field, class_item}] = new_ref_val;

  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      new_stored_values;

  for (const auto& [relname, data] : stored_values) {
    for (const auto& [items, val] : data) {
      T_items new_items(items.size());
      get_relation_items(relname, items.size() - 1, items.back(), new_items);
      new_stored_values[relname][new_items] = val;
    }
  }
  // Return reference_values to its original state.
  reference_values.at(class_name)[{ref_field, class_item}] = old_ref_val;
  return new_stored_values;
}

// Incorporates the data in stored_values (containing updated entity linkages)
// into the relations. New IRM domain clusters may be added.
void GenDB::incorporate_reference(
    std::mt19937* prng,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_values) {
  for (const auto& [rel_name, m] : stored_values) {
    auto f = [&](auto rel) {
      incorporate_reference_relation(prng, rel, rel_name, stored_values);
    };
    std::visit(f, hirm->get_relation(rel_name));
  }
}

template <typename T>
void GenDB::incorporate_reference_relation(
    std::mt19937* prng, Relation<T>* rel, const std::string& rel_name,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_values) {
  if (const T_noisy_relation* trel =
          std::get_if<T_noisy_relation>(&hirm->schema.at(rel_name))) {
    if (stored_values.contains(trel->base_relation)) {
      NoisyRelation<T>* noisy_rel = reinterpret_cast<NoisyRelation<T>*>(rel);
      incorporate_reference_relation(prng, noisy_rel->base_relation,
                                     trel->base_relation, stored_values);
    }
  }
  for (const auto& [items, value] : stored_values.at(rel_name)) {
    // A base relation may have already been reincorporated by a different noisy
    // relation.
    if (!rel->get_data().contains(items)) {
      rel->incorporate(prng, items, std::get<T>(value));
    }
  }
}

// After data has been unincorporated from relations, entities that no longer
// appear in the data may still be incorporated into IRM domain clusters. This
// method detects and unincorporates the entities, and stores their domain
// cluster IDs (so that the entities can later be incorporated into the same
// clusters, if sampled by transition_reference).
double GenDB::unincorporate_from_domain_cluster_relation(
    const std::string& r, const int item, const int ind,
    std::map<std::tuple<int, std::string, T_item>, int>& unincorporated) {
  double logp_adj = 0.;
  int irm_code = hirm->relation_to_table(r);
  IRM* irm = hirm->irms.at(irm_code);
  const std::vector<std::string>& domains =
      std::visit([&](auto trel) { return trel.domains; }, hirm->schema.at(r));
  const std::string& ref_class = domains.at(ind);
  Domain* domain = irm->domains.at(ref_class);

  // Return if: 
  //   - we have already unincorporated this entity from the IRM domain cluster
  //   - we shouldn't unincorporate it because it still exists in the data.
  //   - we can't because it wasn't ever in the data and is therefore not in the cluster.
  if (unincorporated.contains({irm_code, ref_class, item}) ||
      irm->has_observation(ref_class, item) ||
      !domain->items.contains(item)) {
    return logp_adj;
  }

  CRP& crp = domain->crp;
  int cluster_id = crp.assignments.at(item);
  domain->unincorporate(item);
  if (crp.tables.contains(cluster_id)) {
    logp_adj += crp.logp(cluster_id);
  } else {
    logp_adj += log(crp.alpha) - log(crp.N + crp.alpha);
  }
  unincorporated[{irm_code, ref_class, item}] = cluster_id;

  // Recursively check and unincorporate the entity's ancestors.
  if (schema_helper.relation_reference_indices.contains(r) &&
      schema_helper.relation_reference_indices.at(r).contains(ind)) {
    for (auto [name, r_ind] :
         schema_helper.relation_reference_indices.at(r).at(ind)) {
      int ref_item = reference_values.at(ref_class).at({name, item});
      logp_adj += unincorporate_from_domain_cluster_relation(r, ref_item, r_ind,
                                                             unincorporated);
    }
  }
  return logp_adj;
}

// Loops over the reference fields of class_name at class_item and
// unincorporates the reference values from their entity clusters (assuming
// class_name at class_item will be unincorporated). Stores the unincorporated
// values so that they maybe re-incorporated later, if sampled by
// transition_reference. This storage isn't strictly necessary, since we could
// use reference_values to deduce the values that were unincorporated from the
// entity CRPs, but this is a bit more convenient. For the top-level call (from
// transition_reference, is_ancestor_reference=false), we do not accumulate logp
// or unincorporate from the entity CRP, since these are handled separately for
// the reference that's being transitioned.
double GenDB::unincorporate_from_entity_cluster(
    const std::string& class_name, const std::string& ref_field,
    const int class_item,
    std::map<std::tuple<std::string, std::string, int>, int>& unincorporated,
    const bool is_ancestor_reference) {
  double logp_adj = 0.;

  int ref_id = get_reference_id(class_name, ref_field, class_item);
  int ref_item = reference_values.at(class_name).at({ref_field, class_item});

  const std::string& ref_class =
      std::get<ClassVar>(schema.classes.at(class_name).vars.at(ref_field).spec)
          .class_name;
  CRP& crp = domain_crps.at(ref_class);
  if (is_ancestor_reference) {
    crp.unincorporate(ref_id);
    unincorporated[{class_name, ref_field, class_item}] = ref_item;
  }

  if (crp.tables.contains(ref_item)) {
    if (is_ancestor_reference) {
      logp_adj += crp.logp(ref_item);
    }

    for (const auto& [name, var] : schema.classes.at(ref_class).vars) {
      if (std::holds_alternative<ClassVar>(var.spec)) {
        // If this reference value was the only one in its entity_crp table,
        // then the entity is removed from its class and we need to
        // recursively unincorporate *its* reference values.
        logp_adj += unincorporate_from_entity_cluster(ref_class, name, ref_item,
                                                      unincorporated);
      }
    }
  } else {
    if (is_ancestor_reference) {
      logp_adj += crp.logp_new_table();
    }
  }
  return logp_adj;
}

// Recursively unincorporates from base relations and stores values.
template <typename T>
double GenDB::unincorporate_reference_relation_singleton(
    Relation<T>* rel, const std::string& rel_name, const T_items& items,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_value_map) {
  // We can stop unincorporating from base relations if the index of
  // the domain of interest is greater than the number of domains.
  const std::vector<std::string>& domains = std::visit(
      [&](auto tr) { return tr.domains; }, hirm->schema.at(rel_name));
  if (!rel->get_data().contains(items) ||
      domain_crps.at(domains.back()).tables.contains(items.back())) {
    return 0.;
  }

  // Store the items/value and unincorporate them.
  T value = rel->get_value(items);
  stored_value_map[rel_name][items] = value;

  rel->unincorporate_from_cluster(items);
  // Unused prng, since the cluster should exist.
  std::mt19937* prng = nullptr;
  double logp_rel = rel->cluster_or_prior_logp_from_items(prng, items, value);
  // Clean up data but leave empty clusters in case they are re-populated later.
  rel->cleanup_data(items);

  // Recursively unincorporate from base relations.
  if (const T_noisy_relation* t_rel =
          std::get_if<T_noisy_relation>(&hirm->schema.at(rel_name))) {
    auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(rel);
    T_items base_items = noisy_rel->get_base_items(items);
    RelationVariant base_rel = hirm->get_relation(t_rel->base_relation);
    Relation<T>* base_rel_t = std::get<Relation<T>*>(base_rel);
    logp_rel += unincorporate_reference_relation_singleton(
        base_rel_t, t_rel->base_relation, base_items, stored_value_map);
  }
  return logp_rel;
}

// When unincorporating the only reference to a given entity, we additionally
// need to unincorporate data from relations corresponding to attributes of the
// entity's class (or noisy observations of ancestor attributes by the entity's
// class).
double GenDB::unincorporate_singleton(
    const std::string& class_name, const std::string& ref_field,
    const int class_item, const std::string& ref_class,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_value_map,
    std::map<std::tuple<int, std::string, T_item>, int>&
        unincorporated_from_domains,
    std::map<std::tuple<std::string, std::string, int>, int>&
        unincorporated_from_entity_crps) {
  double logp_refclass = 0.;

  int ref_val = reference_values.at(class_name).at({ref_field, class_item});
  logp_refclass +=
      unincorporate_from_entity_cluster(class_name, ref_field, class_item,
                                        unincorporated_from_entity_crps, false);

  for (auto& rel : class_to_relations.at(ref_class)) {
    // CLEAN UP
    const std::vector<std::string>& domains =
        std::visit([&](auto tr) { return tr.domains; }, hirm->schema.at(rel));
    T_items base_items(domains.size());
    get_relation_items(rel, base_items.size() - 1, ref_val, base_items);
    logp_refclass += std::visit(
        [&](auto r) {
          return unincorporate_reference_relation_singleton(r, rel, base_items,
                                                            stored_value_map);
        },
        hirm->get_relation(rel));
  }
  for (auto& rel : class_to_relations.at(ref_class)) {
    // CLEAN UP
    const std::vector<std::string>& domains =
        std::visit([&](auto tr) { return tr.domains; }, hirm->schema.at(rel));
    T_items base_items(domains.size());
    get_relation_items(rel, base_items.size() - 1, ref_val, base_items);
    logp_refclass += unincorporate_from_domain_cluster_relation(
        rel, base_items.back(), base_items.size() - 1,
        unincorporated_from_domains);
  }
  return logp_refclass;
}

void GenDB::transition_reference(std::mt19937* prng,
                                 const std::string& class_name,
                                 const std::string& ref_field,
                                 const int class_item) {
  const std::string& ref_class =
      std::get<ClassVar>(schema.classes.at(class_name).vars.at(ref_field).spec)
          .class_name;
  int init_refval = reference_values.at(class_name).at({ref_field, class_item});
  std::unordered_map<int, double> crp_dist =
      domain_crps[ref_class].tables_weights_gibbs(init_refval);

  // For each relation, get the indices (in the items vector) of the reference
  // value being transitioned.
  std::map<std::string, std::vector<size_t>> domain_inds =
      get_domain_inds(class_name, ref_field);

  // Unincorporate and compute the logp for the current reference value,
  // accounting for relation cluster logps and IRM domain CRP logps. The Gibbs
  // probability of the reference value in its own entity CRP is handled
  // separately. Additional work is done later for singleton reference values.
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_values;  // Stores relation items and values.
  std::map<int, std::map<std::tuple<int, std::string, T_item>, int>>
      unincorporated_from_domains;  // Stores IRM domain cluster IDs.
  double logp_current = unincorporate_reference(
      domain_inds, class_name, ref_field, class_item, stored_values,
      unincorporated_from_domains[init_refval]);

  std::vector<int> entities(crp_dist.size());
  std::vector<double> logps(crp_dist.size(), 0.);

  // Find which entity represents the singleton CRP table -- this is either
  // init_refval or a previously-unseen entity.
  int singleton_entity = init_refval;
  for (const auto [t, w] : crp_dist) {
    if (!domain_crps.at(ref_class).tables.contains(t)) {
      singleton_entity = t;
    }
  }

  // Unincorporate the reference value from its entity CRP. It is important that
  // this is done before the call to unincorporate_singleton.
  int ref_id = get_reference_id(class_name, ref_field, class_item);
  domain_crps.at(ref_class).unincorporate(ref_id);

  // If the current reference value is a singleton (there are no other
  // references to it, and it is the sole member of its entity CRP table), we
  // need to unincorporate its row from the class that it references (including
  // unincorporating all associated data from the relations associated with its
  // attributes).
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      ref_class_relation_stored_values;  // Stores data unincorporated from
                                         // relations.
  std::map<std::tuple<std::string, std::string, int>, int>
      unincorporated_from_entity_crps;
  if (singleton_entity == init_refval) {
    logp_current +=
        unincorporate_singleton(class_name, ref_field, class_item, ref_class,
                                ref_class_relation_stored_values,
                                unincorporated_from_domains[init_refval],
                                unincorporated_from_entity_crps);
  }

  // Loop over the candidate reference values and compute the logp of each.
  int i = 0;
  for (const auto& [table, n_customers] : crp_dist) {
    entities[i] = table;
    logps[i] += log(n_customers);

    reference_values.at(class_name).at({ref_field, class_item}) = table;

    if (table == init_refval) {
      logps[i++] += logp_current;
      continue;
    }

    // Handle the singleton entity, if it was not init_refval (i.e. it is
    // previously unseen).
    if (table == singleton_entity) {
      // Sample and incorporate a new row into the ref_class table. Update
      // reference_values and domain_crps.
      T_items unused_base_items =
          sample_class_ancestors(prng, ref_class, table);

      // Sample and incorporate values into the relations corresponding to
      // the reference class. This may also incorporate new values into the IRM
      // domain clusters.
      for (auto& rel : class_to_relations.at(ref_class)) {
        const std::vector<std::string>& domains = std::visit(
            [&](auto tr) { return tr.domains; }, hirm->schema.at(rel));
        T_items base_items(domains.size());
        get_relation_items(rel, base_items.size() - 1, table, base_items);
        bool c = std::visit(
            [&](auto r) { return r->get_data().contains(base_items); },
            hirm->get_relation(rel));
        if (!c) {
          hirm->sample_and_incorporate_relation(prng, rel, base_items);
        }
      }
    }

    // Get items and values with new entity linkages.
    decltype(stored_values) updated_values_i = update_reference_items(
        stored_values, class_name, ref_field, class_item, table);

    // Incorporate the items/values with new entity linkages into the
    // relations. This may also incorporate new values into the IRM
    // domain clusters.
    incorporate_reference(prng, updated_values_i);

    // Unincorporate the items containing the entity linkage from relations.
    // New entities may have been added to domain clusters in the IRMs;
    // unincorporate them, add up their logp, and store the cluster IDs
    // (so they can be re-incorporated if this table is chosen).
    logps[i] += unincorporate_reference(domain_inds, class_name, ref_field,
                                        class_item, updated_values_i,
                                        unincorporated_from_domains[table]);

    // If table is the singleton, unincorporate and store its references from
    // the entity CRPs. Unincorporate and store the items/values corresponding
    // to the added row in the reference class and their IRM domain clusters.
    if (table == singleton_entity) {
      logps[i] += unincorporate_singleton(
          class_name, ref_field, class_item, ref_class,
          ref_class_relation_stored_values, unincorporated_from_domains[table],
          unincorporated_from_entity_crps);
    }

    ++i;
  }

  // Sample a new reference value.
  int new_ind = sample_from_logps(logps, prng);
  int new_refval = entities.at(new_ind);

  decltype(stored_values) updated_values_new = update_reference_items(
      stored_values, class_name, ref_field, class_item, new_refval);
  if (singleton_entity == new_refval) {
    updated_values_new.merge(ref_class_relation_stored_values);
  }
  reincorporate_new_refval(class_name, ref_field, class_item, new_refval,
                           ref_class, updated_values_new,
                           unincorporated_from_domains.at(new_refval),
                           unincorporated_from_entity_crps);
}

void GenDB::reincorporate_new_refval(
    const std::string& class_name, const std::string& ref_field,
    const int class_item, const int new_refval, const std::string& ref_class,
    std::map<std::string,
             std::unordered_map<T_items, ObservationVariant, H_items>>&
        stored_value_map,
    std::map<std::tuple<int, std::string, T_item>, int>&
        unincorporated_from_domains,
    std::map<std::tuple<std::string, std::string, int>, int>&
        unincorporated_from_entity_crps) {
  // Nothing is sampled anew in this method so prng is unused.
  // std::mt19937* prng = nullptr;
  std::mt19937 prng;  // Debug why empty clusters sometimes get cleaned up.

  // Incorporate the chosen entities back into their previous domain CRPs.
  for (auto [k, v] : unincorporated_from_domains) {
    auto [irm_code, r_class, item] = k;
    if (!hirm->irms.at(irm_code)->domains.at(r_class)->items.contains(item)) {
      hirm->irms.at(irm_code)->domains.at(r_class)->incorporate(&prng, item, v);
    }
  }

  // Update reference_values.
  reference_values.at(class_name).at({ref_field, class_item}) = new_refval;

  // Check if the singleton was selected.
  bool is_singleton = !domain_crps.at(ref_class).tables.contains(new_refval);
  if (is_singleton) {
    // Re-incorporate the references populating the new row in the reference's
    // parent class.
    for (auto [k, v] : unincorporated_from_entity_crps) {
      auto [ref_class, field, item] = k;
      const std::string& subref_class =
          std::get<ClassVar>(schema.classes.at(ref_class).vars.at(field).spec)
              .class_name;
      int ref_id = get_reference_id(ref_class, field, item);
      domain_crps.at(subref_class).incorporate(ref_id, v);
    }
  } else {
    // Remove the singleton from reference_values if it was not selected.
    for (auto [k, v] : unincorporated_from_entity_crps) {
      auto [ref_class, field, item] = k;
      reference_values.at(ref_class).erase({field, item});
    }
  }
  int ref_id = get_reference_id(class_name, ref_field, class_item);
  domain_crps.at(ref_class).incorporate(ref_id, new_refval);

  // Incorporate the items/values with new entity linkages into the relations.
  incorporate_reference(&prng, stored_value_map);

  // Remove empty relation clusters.
  hirm->cleanup_relation_clusters();
}

void GenDB::transition_reference_class_and_ancestors(
    std::mt19937* prng, const std::string& class_name) {
  PCleanClass c = schema.classes.at(class_name);

  for (const auto& [name, var] : c.vars) {
    if (const ClassVar* cv = std::get_if<ClassVar>(&(var.spec))) {
      transition_reference_class_and_ancestors(prng, cv->class_name);
    }
  }

  for (auto [k, v] : reference_values.at(class_name)) {
    auto [ref_field, class_item] = k;
    transition_reference(prng, class_name, ref_field, class_item);
  }
}

GenDB::~GenDB() { delete hirm; }
