// Copyright 2024
// See LICENSE.txt

#pragma once
#include <map>
#include <random>
#include <string>

#include "distributions/crp.hh"
#include "hirm.hh"
#include "observations.hh"
#include "pclean/schema.hh"
#include "pclean/schema_helper.hh"

class GenDB {
 public:
  const PCleanSchema& schema;

  // TODO(emilyaf): Merge PCleanSchemaHelper and GenDB.
  PCleanSchemaHelper schema_helper;

  // This data structure contains entity sets and linkages. Semantics are
  // map<tuple<class_name, reference_field_name, class_primary_key> ref_val>>,
  // where primary_key and ref_val are (integer) entity IDs.
  std::map<std::string, std::map<std::pair<std::string, int>, int>>
      reference_values;

  HIRM* hirm;  // Owned by the GenDB instance.

  // Map keys are class names. Values are CRPs for latent entities, where the
  // "tables" are entity IDs and the "customers" are unique identifiers of
  // observations of that class.
  std::map<std::string, CRP> domain_crps;

  // Maps class names to relations corresponding to attributes of the class.
  std::map<std::string, std::vector<std::string>> class_to_relations;

  GenDB(std::mt19937* prng, const PCleanSchema& schema,
        bool _only_final_emissions = false, bool _record_class_is_clean = true);

  // Return the log probability of the data incorporated into the GenDB so far.
  double logp_score() const;

  // Incorporates a row of observed data into the GenDB instance.
  void incorporate(
      std::mt19937* prng,
      const std::pair<int, std::map<std::string, ObservationVariant>>& row);

  // Incorporates a single element of a row of observed data.
  void incorporate_query_relation(std::mt19937* prng,
                                  const std::string& query_rel,
                                  const T_items& items,
                                  const ObservationVariant& value);

  // Samples a reference value and stores it in reference_values and the
  // relevant domain CRP.
  void sample_and_incorporate_reference(
      std::mt19937* prng, const std::string& class_name,
      const std::pair<std::string, int>& ref_key, const std::string& ref_class);

  // Samples a set of entities in the domains of the relation corresponding to
  // class_path.
  T_items sample_entities_relation(
      std::mt19937* prng, const std::string& class_name,
      std::vector<std::string>::const_iterator class_path_start,
      std::vector<std::string>::const_iterator class_path_end, int class_item);

  void sample_and_incorporate_for_class(std::mt19937* prng, const std::string& class_name, const T_items& items);

  // Sample items from a class' ancestors (recursive reference fields).
  T_items sample_class_ancestors(std::mt19937* prng,
                                 const std::string& class_name, int class_item);

  // Populates "items" with entities by walking the DAG of reference indices,
  // starting with "ind".
  void get_relation_items(const std::string& rel_name, const int ind,
                          const int class_item, T_items& items) const;

  std::map<std::string, std::vector<size_t>> get_domain_inds(
      const std::string& class_name, const std::string& ref_field);

  // Unincorporates all data where class_name refers to ref_field amd has
  // primary key value class_item.
  double unincorporate_reference(
      std::map<std::string, std::vector<size_t>> domain_inds,
      const std::string& class_name, const std::string& ref_field,
      const int class_item,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_value_map,
      std::map<std::tuple<int, std::string, T_item>, int>&
          unincorporated_from_domains);

  // Unincorporates and stores items/values from a relation.
  template <typename T>
  double unincorporate_reference_relation(
      Relation<T>* rel, const std::string& rel_name, const T_items& items,
      const size_t ind,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_value_map);

  template <typename T>
  double unincorporate_reference_relation_singleton(
      Relation<T>* rel, const std::string& rel_name, const T_items& items,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_value_map);

  // Returns a copy of stored_values, with the items updated to associate
  // class_name.ref_field at index class_item with new_ref_val. This update is
  // recursive, such that the reference fields of the class corresponding to
  // ref_field are updated as well. This method behaves as const (although it
  // modifies/restores the state of reference_values).
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
  update_reference_items(
      std::map<std::string, std::unordered_map<T_items, ObservationVariant,
                                               H_items>>& stored_values,
      const std::string& class_name, const std::string& ref_field,
      const int class_item, const int new_ref_val);

  // Incorporates the items and values from stored_values (generally an output
  // of update_reference_items).
  void incorporate_reference(
      std::mt19937* prng,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_values);

  // Recursively incorporates the items and values of stored_values for a single
  // relation (and its base relations).
  template <typename T>
  void incorporate_reference_relation(
      std::mt19937* prng, Relation<T>* rel, const std::string& rel_name,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_values);

  // Returns a unique identifier for a reference value.
  int get_reference_id(const std::string& class_name,
                       const std::string& ref_field, const int class_item);

  // Unincorporates entities from IRM domain clusters.
  double unincorporate_from_domain_cluster_relation(
      const std::string& r, int item, const int ind,
      std::map<std::tuple<int, std::string, T_item>, int>& unincorporated);

  // Unincorporates reference values from their entity clusters.
  double unincorporate_from_entity_cluster(
      const std::string& class_name, const std::string& ref_field,
      const int class_item,
      std::map<std::tuple<std::string, std::string, int>, int>& unincorporated,
      const bool is_ancestor_reference = true);

  // Unincorporates a singleton reference (including, recursively, the
  // corresponding row in the table it points to).
  double unincorporate_singleton(
      const std::string& class_name, const std::string& ref_field,
      const int class_item, const std::string& ref_class,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_value_map,
      std::map<std::tuple<int, std::string, T_item>, int>&
          unincorporated_from_domains,
      std::map<std::tuple<std::string, std::string, int>, int>&
          unincorporated_from_entity_crps);

  // Reincorporates a newly-sampled reference value before returning from the
  // Gibbs kernel.
  void reincorporate_new_refval(
      const std::string& class_name, const std::string& ref_field,
      const int class_item, const int new_refval, const std::string& ref_class,
      std::map<std::string,
               std::unordered_map<T_items, ObservationVariant, H_items>>&
          stored_value_map,
      std::map<std::tuple<int, std::string, T_item>, int>&
          unincorporated_from_domains,
      std::map<std::tuple<std::string, std::string, int>, int>&
          unincorporated_from_entity_crps);

  // Gibbs kernel for transitioning a reference value.
  void transition_reference(std::mt19937* prng, const std::string& class_name,
                            const std::string& ref_field, const int class_item);

  // Transitions all reference fields and rows for a class and its ancestors.
  void transition_reference_class_and_ancestors(std::mt19937* prng,
                                                const std::string& class_name);

  ~GenDB();

  // Disable copying.
  GenDB& operator=(const GenDB&) = delete;
  GenDB(const GenDB&) = delete;
};