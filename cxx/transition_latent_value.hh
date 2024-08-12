// Copyright 2024
// See LICENSE.txt

#pragma once

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include "emissions/base.hh"
#include "noisy_relation.hh"
#include "relation.hh"

typedef int T_item;
typedef std::vector<T_item> T_items;

// Map from noisy relation name to a map from items to noisy observations.
template <typename ValueType>
using T_noisy_observations =
    std::unordered_map<std::string,
                       std::unordered_map<T_items, ValueType, H_items>>;

// This method updates the estimate of a latent value conditioned on noisy
// observations. Cluster assignments are not transitioned. (Temporarily) empty
// clusters are not deleted, and the number of observations in each cluster is
// the same before/after the call to this method.
template <typename ValueType>
void transition_latent_value(
    std::mt19937* prng, T_items base_items, Relation<ValueType>* base_relation,
    std::unordered_map<std::string, NoisyRelation<ValueType>*>
        noisy_relations) {
  printf("transition_latent_value1\n");
  // We incorporate to/unincorporate from clusters themselves rather than
  // relations. Empty clusters are not deleted, because observations will be
  // re-incorporated to them.
  T_noisy_observations<ValueType> noisy_observations =
      unincorporate_and_store_values(base_items, base_relation,
                                     noisy_relations);

  printf("transition_latent_value2\n");
  std::vector<ValueType> all_noisy_observations;
  for (const auto& [name, obs] : noisy_observations) {
    for (const auto& [items, v] : obs) {
      all_noisy_observations.push_back(v);
    }
  }
  printf("transition_latent_value3\n");

  // Candidates for the next latent value are the clean proposals from each
  // cluster in each noisy relation.
  // TODO(emilyaf): Discuss/consider alternatives to generating proposals for
  // the latent values.
  std::vector<ValueType> latent_value_candidates;
  for (auto [name, rel] : noisy_relations) {
    for (const auto& [i, em] : rel->get_emission_clusters()) {
      latent_value_candidates.push_back(
          em->propose_clean(all_noisy_observations, prng));
    }
  }
  printf("transition_latent_value4\n");

  std::vector<double> logpx = latent_values_incremental_logp(
      base_items, latent_value_candidates, noisy_observations, base_relation,
      noisy_relations);
  printf("transition_latent_value5\n");

  choose_and_incorporate_value(prng, base_items, latent_value_candidates, logpx,
                               noisy_observations, base_relation,
                               noisy_relations);
  printf("transition_latent_value6\n");
}

// This function leaves base_relation and noisy_relations in an invalid state,
// since values are unincorporated directly from their clusters.
template <typename ValueType>
T_noisy_observations<ValueType> unincorporate_and_store_values(
    T_items base_items, Relation<ValueType>* base_relation,
    std::unordered_map<std::string, NoisyRelation<ValueType>*>
        noisy_relations) {
  base_relation->unincorporate_from_cluster(base_items);

  // Unincorporate all noisy observations from clusters and store the noisy
  // values.
  T_noisy_observations<ValueType> noisy_observations;
  for (auto [name, rel] : noisy_relations) {
    noisy_observations[name] = {};
    for (const T_items& items : rel->base_to_noisy_items[base_items]) {
      const ValueType& v = rel->get_value(items);
      rel->unincorporate_from_cluster(items);
      noisy_observations[name][items] = v;
    }
  }
  return noisy_observations;
}

// This function assumes that the latent/noisy observations corresponding to
// base_items have been removed from their clusters in
// base_relation/noisy_relations (and returns base_relation/noisy_relations in
// this state as well).
template <typename ValueType>
std::vector<double> latent_values_incremental_logp(
    T_items base_items, std::vector<ValueType> latent_values,
    T_noisy_observations<ValueType> noisy_observations,
    Relation<ValueType>* base_relation,
    std::unordered_map<std::string, NoisyRelation<ValueType>*>
        noisy_relations) {
  printf("latent_values_incremental_logp1\n");
  // Compute logp_score of the noisy relations without the observations of
  // this attribute.
  double baseline_logp_score = 0.;
  for (auto [name, rel] : noisy_relations) {
    baseline_logp_score += rel->logp_score();
  }
  printf("latent_values_incremental_logp2\n");

  std::vector<double> logpx;
  for (const ValueType& v : latent_values) {
    std::cout << "In loop with latent value " << v << "\n";
    // Incorporate the latent/noisy values for this candidate.
    base_relation->incorporate_to_cluster(base_items, v);
    std::cout << "after incorporate_to_cluster\n";
    for (auto& [name, rel] : noisy_relations) {
      printf("considering noisy_relation %s\n", name.c_str());
      for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
        rel->incorporate_to_cluster(items,
                                    noisy_observations.at(name).at(items));
      }
    }
    printf("after noisy relations loop\n");

    // Compute the incremental logp_score of the noisy relations.
    double candidate_logp_score = 0.;
    for (const auto& [name, rel] : noisy_relations) {
      candidate_logp_score += rel->logp_score();
    }
    logpx.push_back(candidate_logp_score - baseline_logp_score);
    printf("after logpx loop\n");

    // Unincorporate the latent/noisy values for this candidate.
    for (auto& [name, rel] : noisy_relations) {
      printf("unincorporating from noisy_relation %s\n", name.c_str());
      for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
        rel->unincorporate_from_cluster(items);
      }
    }
    base_relation->unincorporate_from_cluster(base_items);
  }
  printf("end of latent_values_incremental_logp\n");
  return logpx;
}

// This function assumes observations corresponding to base_items have been
// removed from their clusters in base_relation/noisy_relations. The function
// incorporates new values to these clusters, restoring
// base_relation/noisy_relations to a valid state.
template <typename ValueType>
void choose_and_incorporate_value(
    std::mt19937* prng, T_items base_items,
    std::vector<ValueType> latent_value_candidates, std::vector<double> logpx,
    T_noisy_observations<ValueType> noisy_observations,
    Relation<ValueType>* base_relation,
    std::unordered_map<std::string, NoisyRelation<ValueType>*>
        noisy_relations) {
  // Sample a new latent value.
  ValueType new_latent_value = latent_value_candidates[log_choice(logpx, prng)];

  // Incorporate the new estimate of the latent value.
  base_relation->incorporate_to_cluster(base_items, new_latent_value);
  // Base relation needs to be updated manually since incorporate_to_cluster
  // operates only on the underlying clusters.
  base_relation->update_value(base_items, new_latent_value);
  for (auto& [name, rel] : noisy_relations) {
    for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
      rel->incorporate_to_cluster(items, noisy_observations.at(name).at(items));
    }
  }
}
