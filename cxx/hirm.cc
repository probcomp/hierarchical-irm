// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "hirm.hh"

HIRM::HIRM(const T_schema& schema, std::mt19937* prng) {
  // Add the clean relations before the noisy relations.
  for (const auto& [name, relation] : schema) {
    if (std::holds_alternative<T_clean_relation>(relation)) {
      this->add_relation(prng, name, relation);
    }
  }
  for (const auto& [name, relation] : schema) {
    if (std::holds_alternative<T_noisy_relation>(relation)) {
      this->add_relation(prng, name, relation);
    }
  }
}

void HIRM::incorporate(std::mt19937* prng, const std::string& r,
                       const T_items& items, const ObservationVariant& value) {
  IRM* irm = relation_to_irm(r);
  irm->incorporate(prng, r, items, value);
}

void HIRM::unincorporate(const std::string& r, const T_items& items) {
  IRM* irm = relation_to_irm(r);
  irm->unincorporate(r, items);
}

int HIRM::relation_to_table(const std::string& r) {
  int rc = relation_to_code.at(r);
  return crp.assignments.at(rc);
}

IRM* HIRM::relation_to_irm(const std::string& r) {
  int rc = relation_to_code.at(r);
  int table = crp.assignments.at(rc);
  return irms.at(table);
}

RelationVariant HIRM::get_relation(const std::string& r) {
  IRM* irm = relation_to_irm(r);
  return irm->relations.at(r);
}

void HIRM::transition_cluster_assignments_all(std::mt19937* prng) {
  for (const auto& [r, rc] : relation_to_code) {
    transition_cluster_assignment_relation(prng, r);
  }
}

void HIRM::transition_cluster_assignments(std::mt19937* prng,
                                          const std::vector<std::string>& rs) {
  for (const auto& r : rs) {
    transition_cluster_assignment_relation(prng, r);
  }
}

void HIRM::add_relation_to_irm(IRM* irm, const std::string& r,
                               const T_relation& t_relation) {
  std::visit(
      [&](const auto& trel) {
        using T = std::decay_t<decltype(trel)>;
        if constexpr (std::is_same_v<T, T_clean_relation>) {
          irm->add_relation(r, trel);
        } else if constexpr (std::is_same_v<T, T_noisy_relation>) {
          RelationVariant base_relation = get_relation(trel.base_relation);
          irm->add_relation_with_base(r, trel, base_relation);
        } else {
          assert(false && "Unexpected T_relation variant.");
        }
      },
      t_relation);
}

void HIRM::transition_cluster_assignment_relation(std::mt19937* prng,
                                                  const std::string& r) {
  int rc = relation_to_code.at(r);
  int table_current = crp.assignments.at(rc);
  RelationVariant relation = get_relation(r);
  T_relation t_relation = schema.at(r);
  auto crp_dist = crp.tables_weights_gibbs(table_current);
  std::vector<int> tables;
  std::vector<double> logps;
  int* table_aux = nullptr;
  IRM* irm_aux = nullptr;
  // Compute probabilities of each table.
  for (const auto& [table, n_customers] : crp_dist) {
    IRM* irm;
    if (!irms.contains(table)) {
      irm = new IRM({});
      assert(table_aux == nullptr);
      assert(irm_aux == nullptr);
      table_aux = (int*)malloc(sizeof(*table_aux));
      *table_aux = table;
      irm_aux = irm;
    } else {
      irm = irms.at(table);
    }
    if (table != table_current) {
      add_relation_to_irm(irm, r, t_relation);
      std::visit(
          [&](auto rel) {
            for (const auto& [items, value] : rel->get_data()) {
              irm->incorporate(prng, r, items, value);
            }
          },
          relation);
    }
    RelationVariant rel_r = irm->relations.at(r);
    double lp_data =
        std::visit([](auto rel) { return rel->logp_score(); }, rel_r);
    double lp_crp = log(n_customers);
    logps.push_back(lp_crp + lp_data);
    tables.push_back(table);
  }
  // Sample new table.
  int idx = log_choice(logps, prng);
  T_item choice = tables[idx];

  // Remove relation from all other tables.
  for (const auto& [table, customers] : crp.tables) {
    IRM* irm = irms.at(table);
    if (table != choice) {
      assert(irm->relations.count(r) == 1);
      irm->remove_relation(r);
    }
    if (irm->relations.empty()) {
      assert(crp.tables[table].size() == 1);
      assert(table == table_current);
      irms.erase(table);
      delete irm;
    }
  }
  // Add auxiliary table if necessary.
  if ((table_aux != nullptr) && (choice == *table_aux)) {
    assert(irm_aux != nullptr);
    irms[choice] = irm_aux;
  } else {
    delete irm_aux;
  }
  free(table_aux);
  // Update the CRP.
  crp.unincorporate(rc);
  crp.incorporate(rc, choice);
  assert(irms.size() == crp.tables.size());
  for (const auto& [table, irm] : irms) {
    if (!crp.tables.contains(table)) {
      assert(false);
    }
  }

  // Update any parent relations to point to the new relation as a base
  // relation.
  update_base_relation(r);
}

void HIRM::update_base_relation(const std::string& r) {
  // Updates the pointer to the base relation in each noisy relation.
  if (base_to_noisy_relations.contains(r)) {
    for (const std::string& nrel : base_to_noisy_relations.at(r)) {
      RelationVariant noisy_rel_var = get_relation(nrel);
      std::visit(
          [&](auto nr) {
            using T = typename std::remove_pointer_t<decltype(nr)>::ValueType;
            auto noisy_rel = reinterpret_cast<NoisyRelation<T>*>(nr);
            noisy_rel->base_relation = std::get<Relation<T>*>(get_relation(r));
          },
          noisy_rel_var);
    }
  }
}

void HIRM::set_cluster_assignment_gibbs(std::mt19937* prng,
                                        const std::string& r, int table) {
  assert(irms.size() == crp.tables.size());
  int rc = relation_to_code.at(r);
  int table_current = crp.assignments.at(rc);
  RelationVariant relation = get_relation(r);
  auto f_obs = [&](auto rel) {
    T_relation trel = schema.at(r);
    IRM* irm = relation_to_irm(r);
    auto observations = rel->get_data();
    // Remove from current IRM.
    irm->remove_relation(r);
    if (irm->relations.empty()) {
      irms.erase(table_current);
      delete irm;
    }
    // Add to target IRM.
    if (!irms.contains(table)) {
      irm = new IRM({});
      irms[table] = irm;
    }
    irm = irms.at(table);
    add_relation_to_irm(irm, r, trel);
    for (const auto& [items, value] : observations) {
      irm->incorporate(prng, r, items, value);
    }
  };
  std::visit(f_obs, relation);
  // Update CRP.
  crp.unincorporate(rc);
  crp.incorporate(rc, table);
  update_base_relation(r);
  assert(irms.size() == crp.tables.size());
  for (const auto& [table, irm] : irms) {
    if (!crp.tables.contains(table)) {
      assert(false);
    }
  }
}

void HIRM::add_relation(std::mt19937* prng, const std::string& name,
                        const T_relation& rel) {
  assert(!schema.contains(name));
  schema[name] = rel;
  int offset =
      (code_to_relation.empty())
          ? 0
          : std::max_element(code_to_relation.begin(), code_to_relation.end())
                ->first;
  int rc = 1 + offset;
  int table = crp.sample(prng);
  crp.incorporate(rc, table);
  if (!irms.contains(table)) {
    irms[table] = new IRM({});
  }
  std::visit(
      [&](const auto& trel) {
        using T = std::decay_t<decltype(trel)>;
        if constexpr (std::is_same_v<T, T_noisy_relation>) {
          if (!relation_to_code.contains(trel.base_relation)) {
            add_relation(prng, trel.base_relation,
                         schema.at(trel.base_relation));
          }
          base_to_noisy_relations[trel.base_relation].push_back(name);
        }
      },
      rel);
  add_relation_to_irm(irms.at(table), name, rel);
  assert(!relation_to_code.contains(name));
  assert(!code_to_relation.contains(rc));
  relation_to_code[name] = rc;
  code_to_relation[rc] = name;
}

// Note that remove_relation does not also remove all of the NoisyRelations that
// have the removed relation as a base. The NoisyRelations will be in an invalid
// state after their base relation is removed.
void HIRM::remove_relation(const std::string& name) {
  schema.erase(name);
  int rc = relation_to_code.at(name);
  int table = crp.assignments.at(rc);
  bool singleton = crp.tables.at(table).size() == 1;
  crp.unincorporate(rc);
  irms.at(table)->remove_relation(name);
  if (singleton) {
    IRM* irm = irms.at(table);
    assert(irm->relations.empty());
    irms.erase(table);
    delete irm;
  }
  relation_to_code.erase(name);
  code_to_relation.erase(rc);
}

void HIRM::transition_latent_values_relation(std::mt19937* prng,
                                             const std::string& r) {
  auto transition_values = [&](auto base_rel) {
    using T = typename std::remove_pointer_t<
        std::decay_t<decltype(base_rel)>>::ValueType;
    std::unordered_map<std::string, NoisyRelation<T>*> noisy_rels;
    for (const std::string& name : base_to_noisy_relations.at(r)) {
      noisy_rels[name] = reinterpret_cast<NoisyRelation<T>*>(
          std::get<Relation<T>*>(get_relation(name)));
    }
    for (const auto& [items, value] : base_rel->get_data()) {
      transition_latent_value(prng, items, base_rel, noisy_rels);
    }
  };
  std::visit(transition_values, get_relation(r));
}

double HIRM::logp(
    const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
        observations,
    std::mt19937* prng) {
  std::unordered_map<
      int, std::vector<std::tuple<std::string, T_items, ObservationVariant>>>
      obs_dict;
  for (const auto& [r, items, value] : observations) {
    int rc = relation_to_code.at(r);
    int table = crp.assignments.at(rc);
    if (!obs_dict.contains(table)) {
      obs_dict[table] = {};
    }
    obs_dict.at(table).push_back({r, items, value});
  }
  double logp = 0.0;
  for (const auto& [t, o] : obs_dict) {
    logp += irms.at(t)->logp(o, prng);
  }
  return logp;
}

double HIRM::logp_score() const {
  double logp_score_crp = crp.logp_score();
  double logp_score_irms = 0.0;
  for (const auto& [table, irm] : irms) {
    logp_score_irms += irm->logp_score();
  }
  return logp_score_crp + logp_score_irms;
}

void HIRM::sample_and_incorporate_relation(std::mt19937* prng,
                                           const std::string& r,
                                           T_items& items) {
  // If `r` is a noisy relation, first sample and incorporate to the base
  // relation if necessary.
  if (T_noisy_relation* trel = std::get_if<T_noisy_relation>(&schema.at(r))) {
    std::visit(
        [&](auto nr) {
          using T = typename std::remove_pointer_t<decltype(nr)>::ValueType;
          NoisyRelation<T>* noisy_rel = reinterpret_cast<NoisyRelation<T>*>(nr);
          T_items base_items = noisy_rel->get_base_items(items);
          if (!noisy_rel->base_relation->get_data().contains(base_items)) {
            sample_and_incorporate_relation(prng, trel->base_relation,
                                            base_items);
          }
        },
        get_relation(r));
  }
  std::visit([&](auto rel) { rel->incorporate_sample(prng, items); },
             get_relation(r));
}

void HIRM::sample_and_incorporate(std::mt19937* prng, int n) {
  std::map<std::string, CRP> domain_crps;
  for (const auto& [r, spec] : schema) {
    // If the relation is a leaf, sample n observations of it.
    if (!base_to_noisy_relations.contains(r)) {
      const std::vector<std::string>& r_domains =
          std::visit([](auto trel) { return trel.domains; }, spec);
      int num_samples = 0;
      while (num_samples < n) {
        std::vector<int> entities;
        entities.reserve(r_domains.size());
        for (auto it = r_domains.cbegin(); it != r_domains.cend(); ++it) {
          int entity = domain_crps[*it].sample(prng);
          int crp_item = domain_crps[*it].assignments.size();
          domain_crps[*it].incorporate(crp_item, entity);
          entities.push_back(entity);
        }
        bool r_contains_items = std::visit(
            [&](auto rel) { return rel->get_data().contains(entities); },
            get_relation(r));
        if (!r_contains_items) {
          sample_and_incorporate_relation(prng, r, entities);
          ++num_samples;
        }
      }
    }
  }
}

HIRM::~HIRM() {
  for (const auto& [table, irm] : irms) {
    delete irm;
  }
}
