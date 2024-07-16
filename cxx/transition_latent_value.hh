// Copyright 2024
// See LICENSE.txt

#pragma once

#include <algorithm>
#include <random>
#include <string>
#include <unordered_map>

#include "emissions/base.hh"
#include "noisy_relation.hh"
#include "relation.hh"

typedef int T_item;
typedef std::vector<T_item> T_items;

// This method updates the estimate of a latent value conditioned on noisy
// observations. Cluster assignments are not transitioned. (Temporarily) empty
// clusters are not deleted, and the number of observations in each cluster is
// the same before/after the call to this method.
template <typename ValueType>
void transition_latent_value(
    std::mt19937* prng, T_items base_items, Relation<ValueType>* base_relation,
    std::unordered_map<std::string, NoisyRelation<ValueType>*>
        noisy_relations) {
  // We incorporate to/unincorporate from clusters themselves rather than
  // relations. Empty clusters are not deleted, because observations will be
  // re-incorporated to them.
  base_relation->unincorporate_from_cluster(base_items);

  // Unincorporate all noisy observations from clusters and store the noisy
  // values.
  std::unordered_map<std::string,
                     std::unordered_map<T_items, ValueType, H_items>>
      noisy_observations;
  std::vector<ValueType> all_noisy_observations;
  for (auto [name, rel] : noisy_relations) {
    noisy_observations[name] = {};
    for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
      const ValueType& v = rel->get_value(items);
      rel->unincorporate_from_cluster(items);
      noisy_observations[name][items] = v;
      all_noisy_observations.push_back(v);
    }
  }

  // Compute logp_score of the noisy relations without the observations of
  // this attribute.
  double baseline_logp_score = 0.;
  for (auto [name, rel] : noisy_relations) {
    baseline_logp_score += rel->logp_score();
  }

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

  std::vector<double> logpx;
  for (const ValueType& v : latent_value_candidates) {
    // Incorporate the latent/noisy values for this candidate.
    base_relation->incorporate_to_cluster(base_items, v);
    for (auto& [name, rel] : noisy_relations) {
      for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
        rel->incorporate_to_cluster(items,
                                    noisy_observations.at(name).at(items));
      }
    }

    // Compute the incremental logp_score of the noisy relations.
    double candidate_logp_score = 0.;
    for (const auto& [name, rel] : noisy_relations) {
      candidate_logp_score += rel->logp_score();
    }
    logpx.push_back(candidate_logp_score - baseline_logp_score);

    // Unincorporate the latent/noisy values for this candidate.
    for (auto& [name, rel] : noisy_relations) {
      for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
        rel->unincorporate_from_cluster(items);
      }
    }
    base_relation->unincorporate_from_cluster(base_items);
  }

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
