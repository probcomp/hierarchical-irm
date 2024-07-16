// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "irm.hh"

#include <cstdio>
#include <ctime>
#include <functional>
#include <variant>

RelationVariant clean_relation_from_spec(const std::string& name,
                                         const DistributionSpec& spec,
                                         const std::vector<Domain*>& doms) {
  switch (spec.observation_type) {
    case ObservationEnum::bool_type:
      return new CleanRelation<bool>(name, spec, doms);
    case ObservationEnum::double_type:
      return new CleanRelation<double>(name, spec, doms);
    case ObservationEnum::int_type:
      return new CleanRelation<int>(name, spec, doms);
    case ObservationEnum::string_type:
      return new CleanRelation<std::string>(name, spec, doms);
    default:
      assert(false && "Unsupported observation type.");
  }
}

IRM::IRM(const T_schema& init_schema) {
  assert(schema.empty());
  for (const auto& [name, relation] : init_schema) {
    std::visit([&](auto trel) { this->add_relation(name, trel); }, relation);
  }
  assert(schema.size() == relations.size());
}

IRM::~IRM() {
  for (auto [d, domain] : domains) {
    delete domain;
  }
  for (auto [r, relation] : relations) {
    std::visit([](auto rel) { delete rel; }, relation);
  }
}

void IRM::incorporate(std::mt19937* prng, const std::string& r,
                      const T_items& items, ObservationVariant value) {
  std::visit(
      [&](auto rel) {
        auto v = std::get<
            typename std::remove_reference_t<decltype(*rel)>::ValueType>(value);
        rel->incorporate(prng, items, v);
      },
      relations.at(r));
}

void IRM::unincorporate(const std::string& r, const T_items& items) {
  std::visit([&](auto rel) { rel->unincorporate(items); }, relations.at(r));
}

void IRM::transition_cluster_assignments_all(std::mt19937* prng) {
  for (const auto& [d, domain] : domains) {
    for (const T_item item : domain->items) {
      transition_cluster_assignment_item(prng, d, item);
    }
  }
}

void IRM::transition_cluster_assignments(std::mt19937* prng,
                                         const std::vector<std::string>& ds) {
  for (const std::string& d : ds) {
    for (const T_item item : domains.at(d)->items) {
      transition_cluster_assignment_item(prng, d, item);
    }
  }
}

void IRM::transition_cluster_assignment_item(std::mt19937* prng,
                                             const std::string& d,
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
          rel->logp_gibbs_exact(*domain, item, tables, prng);
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
        rel->set_cluster_assignment_gibbs(*domain, item, choice, prng);
      }
    };
    for (const std::string& r : domain_to_relations.at(d)) {
      std::visit(set_cluster_r, relations.at(r));
    }
    domain->set_cluster_assignment_gibbs(item, choice);
  }
}

double IRM::logp(
    const std::vector<std::tuple<std::string, T_items, ObservationVariant>>&
        observations,
    std::mt19937* prng) {
  std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
      relation_items_seen;
  std::unordered_map<std::string, std::unordered_set<T_item>> domain_item_seen;
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
    int arity = std::visit([](auto rel) { return rel->get_domains().size(); },
                           relation);
    assert(std::ssize(items) == arity);
    for (int i = 0; i < arity; ++i) {
      // Skip if (domain, item) processed.
      Domain* domain = std::visit(
          [&](auto rel) { return rel->get_domains().at(i); }, relation);
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
      for (int i = 0; i < std::ssize(rel->get_domains()); ++i) {
        Domain* domain = rel->get_domains().at(i);
        T_item item = items.at(i);
        auto& [loc, t_list] = cluster_universe.at(domain->name).at(item);
        T_item t = t_list.at(indexes.at(loc));
        z.push_back(t);
      }
      auto v =
          std::get<typename std::remove_reference_t<decltype(*rel)>::ValueType>(
              value);
      return rel->cluster_or_prior_logp(prng, z, v);
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

double IRM::logp_score() const {
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

std::vector<Domain*> IRM::add_domains(
    const std::string& name, const std::vector<std::string>& rel_domains) {
  std::vector<Domain*> doms;
  for (const auto& d : rel_domains) {
    if (domains.count(d) == 0) {
      assert(domain_to_relations.count(d) == 0);
      domains[d] = new Domain(d);
      domain_to_relations[d] = std::unordered_set<std::string>();
    }
    domain_to_relations.at(d).insert(name);
    doms.push_back(domains.at(d));
  }
  return doms;
}

void IRM::add_relation(const std::string& name,
                       const T_clean_relation& relation) {
  assert(!schema.contains(name));
  assert(!relations.contains(name));
  std::vector<Domain*> doms = add_domains(name, relation.domains);

  relations[name] =
      clean_relation_from_spec(name, relation.distribution_spec, doms);
  schema[name] = relation;
}

void IRM::add_relation(const std::string& name,
                       const T_noisy_relation& relation) {
  assert(!schema.contains(name));
  assert(!relations.contains(name));
  std::vector<Domain*> doms = add_domains(name, relation.domains);
  std::string base_name = relation.base_relation;
  // Recursively add base relations.
  RelationVariant base_relation;
  if (!relations.contains(relation.base_relation)) {
    std::visit([&](auto& s) { add_relation(base_name, s); },
               schema.at(base_name));
  }
  base_relation = relations.at(base_name);

  RelationVariant rv;
  std::visit(
      [&](const auto& br) {
        using T = typename std::remove_pointer_t<
            std::decay_t<decltype(br)>>::ValueType;
        rv = new NoisyRelation<T>(name, relation.emission_spec, doms, br);
      },
      base_relation);
  relations[name] = rv;
  schema[name] = relation;
}

void IRM::add_relation_with_base(const std::string& name,
                                 const T_noisy_relation& relation,
                                 RelationVariant base_relation) {
  // Adds a noisy relation from a spec and a base relation pointer.
  // This is meant to be called from HIRM, where the base relation may be in a
  // different IRM.
  assert(!schema.contains(name));
  assert(!relations.contains(name));
  std::vector<Domain*> doms = add_domains(name, relation.domains);

  RelationVariant rv;
  std::visit(
      [&](const auto& br) {
        using T = typename std::remove_pointer_t<
            std::decay_t<decltype(br)>>::ValueType;
        rv = new NoisyRelation<T>(name, relation.emission_spec, doms, br);
      },
      base_relation);
  relations[name] = rv;
  schema[name] = relation;
}

void IRM::remove_relation(const std::string& name) {
  std::unordered_set<std::string> ds;
  auto rel_domains =
      std::visit([](auto r) { return r->get_domains(); }, relations.at(name));
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

#define GET_ELAPSED(t) double(clock() - t) / CLOCKS_PER_SEC

// TODO(emilyaf): Refactor as a function for readibility/maintainability.
#define CHECK_TIMEOUT(timeout, t_begin)           \
  if (timeout) {                                  \
    auto elapsed = GET_ELAPSED(t_begin);          \
    if (timeout < elapsed) {                      \
      printf("timeout after %1.2fs \n", elapsed); \
      break;                                      \
    }                                             \
  }

// TODO(emilyaf): Refactor as a function for readibility/maintainability.
#define REPORT_SCORE(var_verbose, var_t, var_t_total, var_model) \
  if (var_verbose) {                                             \
    auto t_delta = GET_ELAPSED(var_t);                           \
    var_t_total += t_delta;                                      \
    double x = var_model->logp_score();                          \
    printf("%f %f\n", var_t_total, x);                           \
    fflush(stdout);                                              \
  }

void single_step_irm_inference(std::mt19937* prng, IRM* irm, double& t_total,
                               bool verbose, int num_theta_steps) {
  // TRANSITION ASSIGNMENTS.
  for (const auto& [d, domain] : irm->domains) {
    for (const auto item : domain->items) {
      clock_t t = clock();
      irm->transition_cluster_assignment_item(prng, d, item);
      REPORT_SCORE(verbose, t, t_total, irm);
    }
  }
  // TRANSITION DISTRIBUTION HYPERPARAMETERS.
  for (const auto& [r, relation] : irm->relations) {
    std::visit(
        [&](auto r) {
          clock_t t = clock();
          r->transition_cluster_hparams(prng, num_theta_steps);
          REPORT_SCORE(verbose, t, t_total, irm);
        },
        relation);
  }
  // TRANSITION ALPHA.
  for (const auto& [d, domain] : irm->domains) {
    clock_t t = clock();
    domain->crp.transition_alpha(prng);
    REPORT_SCORE(verbose, t, t_total, irm);
  }
}
