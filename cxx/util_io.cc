// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "util_io.hh"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

void load_clean_relation(std::istringstream& stream, T_schema& schema) {
  T_clean_relation relation;

  std::string relname;
  std::string distribution_spec_str;

  stream >> distribution_spec_str;
  stream >> relname;
  for (std::string w; stream >> w;) {
    relation.domains.push_back(w);
  }
  assert(relation.domains.size() > 0);
  relation.distribution_spec = DistributionSpec(distribution_spec_str);

  // If the data contains observations of this relation, this bool will be
  // overwritten to true.
  relation.is_observed = false;
  schema[relname] = relation;
}

void load_noisy_relation(std::istringstream& stream, T_schema& schema) {
  T_noisy_relation relation;

  std::string relname;
  std::string emission_spec_str;
  std::string base_name;

  stream >> emission_spec_str;
  stream >> relname;
  stream >> base_name;
  for (std::string w; stream >> w;) {
    relation.domains.push_back(w);
  }
  assert(relation.domains.size() > 0);
  relation.emission_spec = EmissionSpec(emission_spec_str);
  relation.base_relation = base_name;

  // If the data contains observations of this relation, this bool will be
  // overwritten to true.
  relation.is_observed = false;

  schema[relname] = relation;
}

T_schema load_schema(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  T_schema schema;
  std::string line;
  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string relation_type;
    stream >> relation_type;

    if (relation_type == "clean") {
      load_clean_relation(stream, schema);
    } else if (relation_type == "noisy") {
      load_noisy_relation(stream, schema);
    } else {
      assert(false && "Schema line must start with clean or noisy.");
    }
  }
  fp.close();

  for (const auto& [relname, trelation] : schema)
    std::visit(
        [&](const auto& trel) {
          using T = std::decay_t<decltype(trel)>;
          if constexpr (std::is_same_v<T, T_noisy_relation>) {
            std::vector<std::string> base_domains =
                std::visit([&](const auto& brel) { return brel.domains; },
                           schema.at(trel.base_relation));
            for (size_t i = 0; i != base_domains.size(); ++i) {
              assert(base_domains[i] == trel.domains[i] &&
                     "The first domains of a noisy relation must be the same "
                     "as the domains of the base relation.");
            }
          }
        },
        trelation);
  return schema;
}

T_observations load_observations(const std::string& path, T_schema& schema) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  T_observations observations;
  std::string line;
  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string value_str;
    std::string relname;
    std::vector<std::string> items;

    stream >> value_str;
    stream >> relname;
    ObservationVariant value = observation_string_to_value(
        value_str,
        std::visit(
            [](const auto& trel) {
              using T = std::decay_t<decltype(trel)>;
              if constexpr (std::is_same_v<T, T_clean_relation>) {
                return trel.distribution_spec.observation_type;
              } else if constexpr (std::is_same_v<T, T_noisy_relation>) {
                return trel.emission_spec.observation_type;
              } else {
                assert(false && "Unrecognized relation type.");
              }
            },
            schema.at(relname)));
    for (std::string w; stream >> w;) {
      items.push_back(w);
    }
    assert(items.size() > 0);
    auto entry = std::make_tuple(items, value);
    observations[relname].push_back(entry);
  }
  fp.close();

  // Update the schema to indicate which relations have observations. For now,
  // we assume that each relation is fully observed or fully unobserved.
  for (const auto& [relname, obs] : observations) {
    std::visit([](auto& trel) { trel.is_observed = true; }, schema.at(relname));
  }
  return observations;
}

// Assumes that T_item is integer.
T_encoding encode_observations(const T_schema& schema,
                               const T_observations& observations) {
  // Counter and encoding maps.
  std::map<std::string, int> domain_item_counter;
  T_encoding_f item_to_code;
  T_encoding_r code_to_item;
  // Create a counter of items for each domain.
  for (const auto& [r, relation] : schema) {
    for (const std::string& domain :
         std::visit([](const auto& r) { return r.domains; }, relation)) {
      domain_item_counter[domain] = 0;
      item_to_code[domain] = std::map<std::string, T_item>();
      code_to_item[domain] = std::map<T_item, std::string>();
    }
  }
  // Create the codes for each item.
  for (const auto& [relation, obs] : observations) {
    for (const auto& i : obs) {
      std::vector<std::string> items = std::get<0>(i);
      int counter = 0;
      std::vector<std::string> domains = std::visit(
          [](const auto& r) { return r.domains; }, schema.at(relation));
      for (const std::string& item : items) {
        // Obtain domain that item belongs to.
        std::string domain = domains.at(counter);
        // Compute its code, if necessary.
        if (!item_to_code.at(domain).contains(item)) {
          int code = domain_item_counter[domain];
          item_to_code[domain][item] = code;
          code_to_item[domain][code] = item;
          ++domain_item_counter[domain];
        }
        ++counter;
      }
    }
  }
  return std::make_pair(item_to_code, code_to_item);
}

// Incorporates the observations of a single relation. If the relation is noisy,
// this function recursively incorporates observations of the base relation. If
// the relation is latent and not observed, initial latent values are sampled
// from the relation's cluster prior.
void incorporate_observations_relation(
    std::mt19937* prng, const std::string& relation,
    std::variant<IRM*, HIRM*> h_irm, const T_encoded_observations& observations,
    std::unordered_map<std::string, std::string>& noisy_to_base,
    std::unordered_map<std::string, std::unordered_set<T_items, H_items>>&
        relation_items,
    std::unordered_set<std::string>& completed_relations) {
  RelationVariant rel_var =
      std::visit([&](auto m) { return m->get_relation(relation); }, h_irm);
  // base relations must be incorporated before noisy relations, so recursively
  // incorporate all base relations.
  if (noisy_to_base.contains(relation)) {
    const std::string& base_name = noisy_to_base.at(relation);
    if (!observations.contains(base_name)) {
      // Deduce the base relation items from the noisy relation items, and store
      // them. We will later sample corresponding base relation values.
      std::visit(
          [&](auto rel) {
            using T = typename std::remove_pointer_t<
                std::decay_t<decltype(rel)>>::ValueType;
            NoisyRelation<T>* nrel = reinterpret_cast<NoisyRelation<T>*>(rel);
            for (T_items items : relation_items.at(relation)) {
              T_items base_items = nrel->get_base_items(items);
              relation_items[base_name].insert(base_items);
            }
          },
          rel_var);
    }
    incorporate_observations_relation(prng, base_name, h_irm, observations,
                                      noisy_to_base, relation_items,
                                      completed_relations);
  }

  if (observations.contains(relation)) {
    // If this relation is observed, incorporate its observations.
    for (const auto& [items, value] : observations.at(relation)) {
      std::visit([&](auto& m) { m->incorporate(prng, relation, items, value); },
                 h_irm);
    }
  } else {
    // If this relation is not observed, incorporate samples from the prior.
    // This currently assumes a base relation's items are always a prefix of the
    // noisy relation's items.
    for (const auto& items : relation_items.at(relation)) {
      std::visit(
          [&](auto rel) {
            using T = typename std::remove_pointer_t<
                std::decay_t<decltype(rel)>>::ValueType;
            const auto& rel_data = rel->get_data();
            T value = rel->sample_at_items(prng, items);
            if (!rel_data.contains(items)) {
              std::visit(
                  [&](auto& m) {
                    m->incorporate(prng, relation, items, value);
                  },
                  h_irm);
            }
          },
          rel_var);
    }
  }
  completed_relations.insert(relation);
}

void incorporate_observations(std::mt19937* prng,
                              std::variant<IRM*, HIRM*> h_irm,
                              const T_encoding& encoding,
                              const T_observations& observations) {
  T_encoding_f item_to_code = std::get<0>(encoding);
  std::vector<std::string> observed_relations;
  T_encoded_observations encoded_observations;
  std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
      relation_items;
  for (const auto& [relation, obs] : observations) {
    observed_relations.push_back(relation);

    T_relation trel = std::visit(
        [&](const auto& m) { return m->schema.at(relation); }, h_irm);
    std::vector<std::string> domains =
        std::visit([&](const auto& tr) { return tr.domains; }, trel);
    for (const auto& [items, value] : obs) {
      int counter = 0;
      T_items items_e;
      for (const std::string& item : items) {
        std::string domain = domains[counter];
        counter += 1;
        int code = item_to_code.at(domain).at(item);
        items_e.push_back(code);
      }
      relation_items[relation].insert(items_e);
      encoded_observations[relation].push_back(std::make_pair(items_e, value));
    }
  }

  std::unordered_map<std::string, std::string> noisy_to_base;
  std::unordered_map<std::string, std::vector<std::string>> base_to_noisy =
      std::visit([](const auto& m) { return m->base_to_noisy_relations; },
                 h_irm);
  for (const auto& [base_name, noisy_names] : base_to_noisy) {
    for (const std::string& noisy_name : noisy_names) {
      if (!base_to_noisy.contains(noisy_name)) {
        assert(observations.contains(noisy_name) &&
               "A relation that is not the base of a noisy relation must be "
               "observed.");
      }
      noisy_to_base[noisy_name] = base_name;
    }
  }

  std::unordered_set<std::string> completed_relations;
  for (const std::string& relation : observed_relations) {
    if (!completed_relations.contains(relation)) {
      incorporate_observations_relation(prng, relation, h_irm,
                                        encoded_observations, noisy_to_base,
                                        relation_items, completed_relations);
    }
  }
}

void to_txt(std::ostream& fp, const IRM& irm, const T_encoding& encoding) {
  T_encoding_r code_to_item = std::get<1>(encoding);
  for (const auto& [d, domain] : irm.domains) {
    auto i0 = domain->crp.tables.begin();
    auto i1 = domain->crp.tables.end();
    std::map<int, std::unordered_set<T_item>> tables(i0, i1);
    for (const auto& [table, items] : tables) {
      fp << domain->name << " ";
      fp << table << " ";
      int i = 1;
      for (const T_item& item : items) {
        fp << code_to_item.at(domain->name).at(item);
        if (i++ < std::ssize(items)) {
          fp << " ";
        }
      }
      fp << "\n";
    }
  }
}

void to_txt(std::ostream& fp, const HIRM& hirm, const T_encoding& encoding) {
  // Write the relation clusters.
  auto i0 = hirm.crp.tables.begin();
  auto i1 = hirm.crp.tables.end();
  std::map<int, std::unordered_set<T_item>> tables(i0, i1);
  for (const auto& [table, rcs] : tables) {
    fp << table << " ";
    int i = 1;
    for (const T_item rc : rcs) {
      fp << hirm.code_to_relation.at(rc);
      if (i++ < std::ssize(rcs)) {
        fp << " ";
      }
    }
    fp << "\n";
  }
  fp << "\n";
  // Write the IRMs.
  int j = 0;
  for (const auto& [table, rcs] : tables) {
    const IRM* const irm = hirm.irms.at(table);
    fp << "irm=" << table << "\n";
    to_txt(fp, *irm, encoding);
    if (j < std::ssize(tables) - 1) {
      fp << "\n";
      j += 1;
    }
  }
}

void to_txt(const std::string& path, const IRM& irm,
            const T_encoding& encoding) {
  std::ofstream fp(path);
  assert(fp.good());
  to_txt(fp, irm, encoding);
  fp.close();
}

void to_txt(const std::string& path, const HIRM& hirm,
            const T_encoding& encoding) {
  std::ofstream fp(path);
  assert(fp.good());
  to_txt(fp, hirm, encoding);
  fp.close();
}

std::map<std::string, std::map<int, std::vector<std::string>>>
load_clusters_irm(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  std::map<std::string, std::map<int, std::vector<std::string>>> clusters;
  std::string line;
  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string domain;
    int table;
    std::vector<std::string> items;

    stream >> domain;
    stream >> table;
    for (std::string w; stream >> w;) {
      items.push_back(w);
    }
    assert(items.size() > 0);
    assert(clusters[domain].count(table) == 0);
    clusters[domain][table] = items;
  }
  fp.close();
  return clusters;
}

int isnumeric(const std::string& s) {
  for (char c : s) {
    if (!isdigit(c)) {
      return false;
    }
  }
  return !s.empty() && true;
}

std::tuple<std::map<int, std::vector<std::string>>,  // x[table] = {relation
                                                     // list}
           std::map<
               int,
               std::map<
                   std::string,
                   std::map<int, std::vector<
                                     std::string>>>>  // x[table][domain][table]
                                                      // =
                                                      // {item
                                                      // list}
           >
load_clusters_hirm(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  std::map<int, std::vector<std::string>> relations;
  std::map<int, std::map<std::string, std::map<int, std::vector<std::string>>>>
      irms;

  std::string line;
  int irmc = 0;

  while (std::getline(fp, line)) {
    std::istringstream stream(line);

    std::string first;
    stream >> first;

    // Parse a relation cluster.
    if (isnumeric(first)) {
      int table = std::stoi(first);
      std::vector<std::string> items;
      for (std::string item; stream >> item;) {
        items.push_back(item);
      }
      assert(items.size() > 0);
      assert(relations.count(table) == 0);
      relations[table] = items;
      continue;
    }

    // Skip a new line.
    if (first.size() == 0) {
      irmc = -1;
      continue;
    }

    // Parse an irm= line.
    if (first.rfind("irm=", 0) == 0) {
      assert(irmc == -1);
      assert(first.size() > 4);
      std::string x = first.substr(4);
      irmc = std::stoi(x);
      assert(irms.count(irmc) == 0);
      irms[irmc] = {};
      continue;
    }

    // Parse a domain cluster.
    assert(irmc > -1);
    assert(irms.count(irmc) == 1);
    std::string second;
    stream >> second;
    assert(second.size() > 0);
    assert(isnumeric(second));
    std::string& domain = first;
    int table = std::stoi(second);
    std::vector<std::string> items;
    for (std::string item; stream >> item;) {
      items.push_back(item);
    }
    assert(items.size() > 0);
    if (irms.at(irmc).count(domain) == 0) {
      irms.at(irmc)[domain] = {};
    }
    assert(irms.at(irmc).at(domain).count(table) == 0);
    irms.at(irmc).at(domain)[table] = items;
  }

  assert(relations.size() == irms.size());
  for (const auto& [t, rs] : relations) {
    assert(irms.count(t) == 1);
  }
  fp.close();
  return std::make_pair(relations, irms);
}

void from_txt(std::mt19937* prng, IRM* const irm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters) {
  // Load the data.
  T_schema schema = load_schema(path_schema);
  T_observations observations = load_observations(path_obs, schema);
  T_encoding encoding = encode_observations(schema, observations);
  auto clusters = load_clusters_irm(path_clusters);
  // Add the relations.
  assert(irm->schema.empty());
  assert(irm->domains.empty());
  assert(irm->relations.empty());
  assert(irm->domain_to_relations.empty());
  for (const auto& [r, ds] : schema) {
    std::visit([&](const auto& trel) { irm->add_relation(r, trel); }, ds);
  }
  // Add the domain entities with fixed clustering.
  T_encoding_f item_to_code = std::get<0>(encoding);
  for (const auto& [domain, tables] : clusters) {
    assert(irm->domains.at(domain)->items.size() == 0);
    for (const auto& [table, items] : tables) {
      assert(0 <= table);
      for (const std::string& item : items) {
        T_item code = item_to_code.at(domain).at(item);
        irm->domains.at(domain)->incorporate(prng, code, table);
      }
    }
  }
  // Add the observations.
  incorporate_observations(prng, irm, encoding, observations);
}

void from_txt(std::mt19937* prng, HIRM* const hirm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters) {
  T_schema schema = load_schema(path_schema);
  T_observations observations = load_observations(path_obs, schema);
  T_encoding encoding = encode_observations(schema, observations);
  auto [relations, irms] = load_clusters_hirm(path_clusters);
  // Add the relations.
  assert(hirm->schema.empty());
  assert(hirm->irms.empty());
  assert(hirm->relation_to_code.empty());
  assert(hirm->code_to_relation.empty());
  for (const auto& [r, ds] : schema) {
    hirm->add_relation(prng, r, ds);
    assert(hirm->irms.size() == hirm->crp.tables.size());
    hirm->set_cluster_assignment_gibbs(prng, r, -1);
  }
  // Add each IRM.
  for (const auto& [table, rs] : relations) {
    assert(hirm->irms.size() == hirm->crp.tables.size());
    // Add relations to the IRM.
    for (const std::string& r : rs) {
      assert(hirm->irms.size() == hirm->crp.tables.size());
      int table_current = hirm->relation_to_table(r);
      if (table_current != table) {
        assert(hirm->irms.size() == hirm->crp.tables.size());
        hirm->set_cluster_assignment_gibbs(prng, r, table);
      }
    }
    // Add the domain entities with fixed clustering to this IRM.
    // TODO: Duplicated code with from_txt(IRM)
    IRM* irm = hirm->irms.at(table);
    auto clusters = irms.at(table);
    assert(irm->relations.size() == rs.size());
    T_encoding_f item_to_code = std::get<0>(encoding);
    for (const auto& [domain, tables] : clusters) {
      assert(irm->domains.at(domain)->items.size() == 0);
      for (const auto& [t, items] : tables) {
        assert(0 <= t);
        for (const std::string& item : items) {
          int code = item_to_code.at(domain).at(item);
          irm->domains.at(domain)->incorporate(prng, code, t);
        }
      }
    }
  }
  assert(hirm->irms.count(-1) == 0);
  // Add the observations.
  incorporate_observations(prng, hirm, encoding, observations);
}
