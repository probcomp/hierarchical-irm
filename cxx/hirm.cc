// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "hirm.hh"

HIRM::HIRM(const T_schema& schema, std::mt19937* prng) {
  for (const auto& [name, relation] : schema) {
    this->add_relation(prng, name, relation);
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

void HIRM::transition_cluster_assignment_relation(std::mt19937* prng,
                                                  const std::string& r) {
  int rc = relation_to_code.at(r);
  int table_current = crp.assignments.at(rc);
  RelationVariant relation = get_relation(r);
  T_relation t_relation =
      std::visit([](auto rel) { return rel->trel; }, relation);
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
      irm->add_relation(r, t_relation);
      std::visit(
          [&](auto rel) {
            for (const auto& [items, value] : rel->data) {
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
    assert(crp.tables.contains(table));
  }
}

void HIRM::set_cluster_assignment_gibbs(
    std::mt19937* prng, const std::string& r, int table) {
  assert(irms.size() == crp.tables.size());
  int rc = relation_to_code.at(r);
  int table_current = crp.assignments.at(rc);
  RelationVariant relation = get_relation(r);
  auto f_obs = [&](auto rel) {
    T_relation trel = rel->trel;
    IRM* irm = relation_to_irm(r);
    auto observations = rel->data;
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
    irm->add_relation(r, trel);
    for (const auto& [items, value] : observations) {
      irm->incorporate(prng, r, items, value);
    }
  };
  std::visit(f_obs, relation);
  // Update CRP.
  crp.unincorporate(rc);
  crp.incorporate(rc, table);
  assert(irms.size() == crp.tables.size());
  for (const auto& [table, irm] : irms) {
    assert(crp.tables.contains(table));
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
  if (irms.count(table) == 1) {
    irms.at(table)->add_relation(name, rel);
  } else {
    irms[table] = new IRM({{name, rel}});
  }
  assert(!relation_to_code.contains(name));
  assert(!code_to_relation.contains(rc));
  relation_to_code[name] = rc;
  code_to_relation[rc] = name;
}

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

double HIRM::logp(
    const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
    observations) {
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
    logp += irms.at(t)->logp(o);
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

HIRM::~HIRM() {
  for (const auto& [table, irm] : irms) {
    delete irm;
  }
}

