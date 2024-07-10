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

template <typename ValueType>
class Attribute {
 public:
  // Items corresponding to the true (unobserved) value stored in base_relation.
  T_items base_items;

  // Pointer to Relation where base_items and the true value are incorporated.
  Relation<ValueType>* base_relation;

  // Maps a relation name to a pointer to the NoisyRelation containing the
  // observed noisy values.
  std::unordered_map<std::string, NoisyRelation<ValueType>*> noisy_relations;

  Attribute(T_items base_items, Relation<ValueType>* base_relation,
            std::unordered_map<std::string, NoisyRelation<ValueType>*>
                noisy_relations)
      : base_items(base_items),
        base_relation(base_relation),
        noisy_relations(noisy_relations){
          for (const auto& [name, rel] : noisy_relations) {
            assert(rel->base_relation == base_relation);
            const auto& base_data = base_relation->get_data();
            assert(base_data.contains(base_items));
          }
        };

  void transition_true_value(std::mt19937* prng) {
    // Since this method does not transition cluster assignments, we incorporate
    // to/unincorporate from clusters themselves rather than relations. Empty
    // clusters are not deleted, because observations will be re-incorporated to
    // them. The number of observations in each cluster is the same before/after
    // the call to this method.
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

    // Candidates for the next "true" value are the clean proposals from each
    // cluster in each noisy relation.
    std::vector<ValueType> true_value_candidates;
    for (auto [name, rel] : noisy_relations) {
      for (const auto& [i, em] : rel->emission_relation.clusters) {
        true_value_candidates.push_back(
            reinterpret_cast<Emission<ValueType>*>(em)->propose_clean(
                all_noisy_observations, prng));
      }
    }

    std::vector<double> logpx;
    for (const ValueType& v : true_value_candidates) {
      // Incorporate the true/noisy values for this candidate.
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

      // Unincorporate the true/noisy values for this candidate.
      for (auto& [name, rel] : noisy_relations) {
        for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
          rel->unincorporate_from_cluster(items);
        }
      }
      base_relation->unincorporate_from_cluster(base_items);
    }
    
    // Sample a new latent value.
    ValueType new_true_value = true_value_candidates[log_choice(logpx, prng)];

    // Incorporate the new estimate for the true value.
    base_relation->incorporate_to_cluster(base_items, new_true_value);
    // Base relation needs to be updated manually since incorporate_to_cluster
    // operates only on the underlying clusters.
    base_relation->update_value(base_items, new_true_value);
    for (auto& [name, rel] : noisy_relations) {
      for (const T_items& items : rel->base_to_noisy_items.at(base_items)) {
        rel->incorporate_to_cluster(items,
                                    noisy_observations.at(name).at(items));
      }
    }
  }
};