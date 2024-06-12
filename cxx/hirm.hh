// Copyright 2020
// See LICENSE.txt

#pragma once
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "relation.hh"
#include "util_distribution_variant.hh"


// Map from names to T_relation's.
typedef std::map<std::string, T_relation> T_schema;

class IRM {
 public:
  T_schema schema;                                   // schema of relations
  std::unordered_map<std::string, Domain*> domains;  // map from name to Domain
  std::unordered_map<std::string, RelationVariant>
      relations;  // map from name to Relation
  std::unordered_map<std::string, std::unordered_set<std::string>>
      domain_to_relations;  // reverse map
  std::mt19937* prng;

  IRM(const T_schema& schema, std::mt19937* prng) {
    this->prng = prng;
    for (const auto& [name, relation] : schema) {
      this->add_relation(name, relation);
    }
  }

  ~IRM() {
    for (auto [d, domain] : domains) {
      delete domain;
    }
    for (auto [r, relation] : relations) {
      std::visit([](auto rel) { delete rel; }, relation);
    }
  }

  void incorporate(const std::string& r, const T_items& items,
                   ObservationVariant value) {
    std::visit(
        [&](auto rel) {
          auto v = std::get<
              typename std::remove_reference_t<decltype(*rel)>::ValueType>(
              value);
          rel->incorporate(items, v);
        },
        relations.at(r));
  }

  void unincorporate(const std::string& r, const T_items& items) {
    std::visit([&](auto rel) { rel->unincorporate(items); }, relations.at(r));
  }

  void transition_cluster_assignments_all() {
    for (const auto& [d, domain] : domains) {
      for (const T_item item : domain->items) {
        transition_cluster_assignment_item(d, item);
      }
    }
  }

  void transition_cluster_assignments(const std::vector<std::string>& ds) {
    for (const std::string& d : ds) {
      for (const T_item item : domains.at(d)->items) {
        transition_cluster_assignment_item(d, item);
      }
    }
  }

  void transition_cluster_assignment_item(const std::string& d,
                                          const T_item& item) {
    Domain* domain = domains.at(d);
    auto crp_dist = domain->tables_weights_gibbs(item);
    // Compute probability of each table.
    std::vector<int> tables;
    std::vector<double> logps;
    tables.reserve(crp_dist.size());
    logps.reserve(crp_dist.size());
    for (const auto& [table, n_customers] : crp_dist) {
      tables.push_back(table);
      logps.push_back(log(n_customers));
    }
    auto accumulate_logps = [&](auto rel) {
      if (rel->has_observation(*domain, item)) {
        std::vector<double> lp_relation =
            rel->logp_gibbs_exact(*domain, item, tables);
        assert(lp_relation.size() == tables.size());
        assert(lp_relation.size() == logps.size());
        for (int i = 0; i < std::ssize(logps); ++i) {
          logps[i] += lp_relation[i];
        }
      }
    };
    for (const auto& r : domain_to_relations.at(d)) {
      std::visit(accumulate_logps, relations.at(r));
    }
    // Sample new table.
    assert(tables.size() == logps.size());
    int idx = log_choice(logps, prng);
    T_item choice = tables[idx];
    // Move to new table (if necessary).
    if (choice != domain->get_cluster_assignment(item)) {
      auto set_cluster_r = [&](auto rel) {
        if (rel->has_observation(*domain, item)) {
          rel->set_cluster_assignment_gibbs(*domain, item, choice);
        }
      };
      for (const std::string& r : domain_to_relations.at(d)) {
        std::visit(set_cluster_r, relations.at(r));
      }
      domain->set_cluster_assignment_gibbs(item, choice);
    }
  }

  double logp(
      const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
          observations) {
    std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
        relation_items_seen;
    std::unordered_map<std::string, std::unordered_set<T_item>>
        domain_item_seen;
    std::vector<std::tuple<std::string, T_item>> item_universe;
    std::vector<std::vector<int>> index_universe;
    std::vector<std::vector<double>> weight_universe;
    std::unordered_map<
        std::string,
        std::unordered_map<T_item, std::tuple<int, std::vector<int>>>>
        cluster_universe;
    // Compute all cluster combinations.
    for (const auto& [r, items, value] : observations) {
      // Assert observation is unique.
      assert(!relation_items_seen[r].contains(items));
      relation_items_seen[r].insert(items);
      // Process each (domain, item) in the observations.
      RelationVariant relation = relations.at(r);
      int arity =
          std::visit([](auto rel) { return rel->domains.size(); }, relation);
      assert(std::ssize(items) == arity);
      for (int i = 0; i < arity; ++i) {
        // Skip if (domain, item) processed.
        Domain* domain =
            std::visit([&](auto rel) { return rel->domains.at(i); }, relation);
        T_item item = items.at(i);
        if (domain_item_seen[domain->name].contains(item)) {
          assert(cluster_universe[domain->name].contains(item));
          continue;
        }
        domain_item_seen[domain->name].insert(item);
        // Obtain tables, weights, indexes for this item.
        std::vector<int> t_list;
        std::vector<double> w_list;
        std::vector<int> i_list;
        size_t n_tables = domain->tables_weights().size() + 1;
        t_list.reserve(n_tables);
        w_list.reserve(n_tables);
        i_list.reserve(n_tables);
        if (domain->items.contains(item)) {
          int z = domain->get_cluster_assignment(item);
          t_list = {z};
          w_list = {0.0};
          i_list = {0};
        } else {
          auto tables_weights = domain->tables_weights();
          double Z = log(domain->crp.alpha + domain->crp.N);
          size_t idx = 0;
          for (const auto& [t, w] : tables_weights) {
            t_list.push_back(t);
            w_list.push_back(log(w) - Z);
            i_list.push_back(idx++);
          }
          assert(idx == t_list.size());
        }
        // Add to universe.
        item_universe.push_back({domain->name, item});
        index_universe.push_back(i_list);
        weight_universe.push_back(w_list);
        int loc = index_universe.size() - 1;
        cluster_universe[domain->name][item] = {loc, t_list};
      }
    }
    assert(item_universe.size() == index_universe.size());
    assert(item_universe.size() == weight_universe.size());
    // Compute data probability given cluster combinations.
    std::vector<T_items> items_product = product(index_universe);
    std::vector<double> logps;  // reserve size
    logps.reserve(index_universe.size());
    for (const T_items& indexes : items_product) {
      double logp_indexes = 0;
      // Compute weight of cluster assignments.
      double weight = 0.0;
      for (int i = 0; i < std::ssize(indexes); ++i) {
        weight += weight_universe.at(i).at(indexes[i]);
      }
      logp_indexes += weight;
      // Compute weight of data given cluster assignments.
      auto f_logp = [&](auto rel, const T_items& items,
                        const ObservationVariant& value) -> double {
        std::vector<int> z;
        z.reserve(domains.size());
        for (int i = 0; i < std::ssize(rel->domains); ++i) {
          Domain* domain = rel->domains.at(i);
          T_item item = items.at(i);
          auto& [loc, t_list] = cluster_universe.at(domain->name).at(item);
          T_item t = t_list.at(indexes.at(loc));
          z.push_back(t);
        }
        auto v = std::get<
            typename std::remove_reference_t<decltype(*rel)>::ValueType>(value);
        return rel->clusters.contains(z) ? rel->clusters.at(z)->logp(v) 
                                         : rel->cluster_prior->logp(v);
      };
      for (const auto& [r, items, value] : observations) {
        auto g = std::bind(f_logp, std::placeholders::_1, items, value);
        double logp_obs = std::visit(g, relations.at(r));
        logp_indexes += logp_obs;
      };
      logps.push_back(logp_indexes);
    }
    return logsumexp(logps);
  }

  double logp_score() const {
    double logp_score_crp = 0.0;
    for (const auto& [d, domain] : domains) {
      logp_score_crp += domain->crp.logp_score();
    }
    double logp_score_relation = 0.0;
    for (const auto& [r, relation] : relations) {
      double logp_rel =
          std::visit([](auto rel) { return rel->logp_score(); }, relation);
      logp_score_relation += logp_rel;
    }
    return logp_score_crp + logp_score_relation;
  }

  void add_relation(const std::string& name, const T_relation& relation) {
    assert(!schema.contains(name));
    assert(!relations.contains(name));
    std::vector<Domain*> doms;
    for (const auto& d : relation.domains) {
      if (domains.count(d) == 0) {
        assert(domain_to_relations.count(d) == 0);
        domains[d] = new Domain(d, prng);
        domain_to_relations[d] = std::unordered_set<std::string>();
      }
      domain_to_relations.at(d).insert(name);
      doms.push_back(domains.at(d));
    }
    relations[name] = relation_from_spec(name, relation.distribution_spec, doms, prng);
    schema[name] = relation;
  }

  void remove_relation(const std::string& name) {
    std::unordered_set<std::string> ds;
    auto rel_domains =
        std::visit([](auto r) { return r->domains; }, relations.at(name));
    for (const Domain* const domain : rel_domains) {
      ds.insert(domain->name);
    }
    for (const auto& d : ds) {
      domain_to_relations.at(d).erase(name);
      // TODO: Remove r from domains.at(d)->items
      if (domain_to_relations.at(d).empty()) {
        domain_to_relations.erase(d);
        delete domains.at(d);
        domains.erase(d);
      }
    }
    std::visit([](auto r) { delete r; }, relations.at(name));
    relations.erase(name);
    schema.erase(name);
  }

  // Disable copying.
  IRM& operator=(const IRM&) = delete;
  IRM(const IRM&) = delete;
};

class HIRM {
 public:
  T_schema schema;                     // schema of relations
  std::unordered_map<int, IRM*> irms;  // map from cluster id to IRM
  std::unordered_map<std::string, int>
      relation_to_code;  // map from relation name to code
  std::unordered_map<int, std::string>
      code_to_relation;  // map from code to relation
  CRP crp;               // clustering model for relations
  std::mt19937* prng;

  HIRM(const T_schema& schema, std::mt19937* prng) : crp(prng) {
    this->prng = prng;
    for (const auto& [name, relation] : schema) {
      this->add_relation(name, relation);
    }
  }

  void incorporate(const std::string& r, const T_items& items, 
                   const ObservationVariant& value) {
    IRM* irm = relation_to_irm(r);
    irm->incorporate(r, items, value);
  }
  void unincorporate(const std::string& r, const T_items& items) {
    IRM* irm = relation_to_irm(r);
    irm->unincorporate(r, items);
  }

  int relation_to_table(const std::string& r) {
    int rc = relation_to_code.at(r);
    return crp.assignments.at(rc);
  }
  IRM* relation_to_irm(const std::string& r) {
    int rc = relation_to_code.at(r);
    int table = crp.assignments.at(rc);
    return irms.at(table);
  }
  RelationVariant get_relation(const std::string& r) {
    IRM* irm = relation_to_irm(r);
    return irm->relations.at(r);
  }

  void transition_cluster_assignments_all() {
    for (const auto& [r, rc] : relation_to_code) {
      transition_cluster_assignment_relation(r);
    }
  }
  void transition_cluster_assignments(const std::vector<std::string>& rs) {
    for (const auto& r : rs) {
      transition_cluster_assignment_relation(r);
    }
  }
  void transition_cluster_assignment_relation(const std::string& r) {
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
        irm = new IRM({}, prng);
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
                irm->incorporate(r, items, value);
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

  void set_cluster_assignment_gibbs(const std::string& r, int table) {
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
        irm = new IRM({}, prng);
        irms[table] = irm;
      }
      irm = irms.at(table);
      irm->add_relation(r, trel);
      for (const auto& [items, value] : observations) {
        irm->incorporate(r, items, value);
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

  void add_relation(const std::string& name, const T_relation& rel) {
    assert(!schema.contains(name));
    schema[name] = rel;
    int offset =
        (code_to_relation.empty())
            ? 0
            : std::max_element(code_to_relation.begin(), code_to_relation.end())
                  ->first;
    int rc = 1 + offset;
    int table = crp.sample();
    crp.incorporate(rc, table);
    if (irms.count(table) == 1) {
      irms.at(table)->add_relation(name, rel);
    } else {
      irms[table] = new IRM({{name, rel}}, prng);
    }
    assert(!relation_to_code.contains(name));
    assert(!code_to_relation.contains(rc));
    relation_to_code[name] = rc;
    code_to_relation[rc] = name;
  }
  void remove_relation(const std::string& name) {
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

  double logp(
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

  double logp_score() {
    double logp_score_crp = crp.logp_score();
    double logp_score_irms = 0.0;
    for (const auto& [table, irm] : irms) {
      logp_score_irms += irm->logp_score();
    }
    return logp_score_crp + logp_score_irms;
  }

  ~HIRM() {
    for (const auto& [table, irm] : irms) {
      delete irm;
    }
  }

  // Disable copying.
  HIRM& operator=(const HIRM&) = delete;
  HIRM(const HIRM&) = delete;
};
