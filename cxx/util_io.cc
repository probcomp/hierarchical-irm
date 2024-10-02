// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "util_io.hh"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "util_parse.hh"

void verify_noisy_relation_domains(const T_schema& schema) {
  for (const auto& [relname, trelation] : schema) {
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
  }
}

#define TYPE_CHECK(i, expected_type)                                        \
  if (i >= tokens.size()) {                                                 \
    printf("Error parsing schema line %s: expected at least %ld tokens\n",  \
           line.c_str(), i + 1);                                            \
    assert(false);                                                          \
  }                                                                         \
  if (tokens[i].type != TokenType::expected_type) {                         \
    printf(                                                                 \
        "Error parsing schema line %s:  expected expected_type for %ld-th " \
        "token\n",                                                          \
        line.c_str(), i + 1);                                               \
    printf("Got %s of type %d instead\n", tokens[0].val.c_str(),            \
           (int)(tokens[0].type));                                          \
    assert(false);                                                          \
  }

#define VAL_CHECK(i, expected_val)                                          \
  if (i >= tokens.size()) {                                                 \
    printf("Error parsing schema line %s: expected at least %ld tokens\n",  \
           line.c_str(), i + 1);                                            \
    assert(false);                                                          \
  }                                                                         \
  if (tokens[i].val != expected_val) {                                      \
    printf("Error parsing schema line %s:  expected %s for %ld-th token\n", \
           line.c_str(), expected_val, i + 1);                              \
    printf("Got %s of type %d instead\n", tokens[0].val.c_str(),            \
           (int)(tokens[0].type));                                          \
    assert(false);                                                          \
  }

T_schema load_schema(const std::string& path) {
  std::ifstream fp(path, std::ifstream::in);
  assert(fp.good());

  T_schema schema;
  std::string line;
  std::vector<Token> tokens;
  while (std::getline(fp, line)) {
    tokens.clear();
    [[maybe_unused]] bool ok = tokenize(line, &tokens);
    assert(ok);

    if (tokens.empty()) {
      continue;
    }

    size_t i = 0;
    TYPE_CHECK(i, identifier);
    std::string relname = tokens[i].val;

    i = 1;
    VAL_CHECK(i, "~");

    i = 2;
    TYPE_CHECK(i, identifier);
    std::string dist_name = tokens[2].val;

    i = 3;
    std::map<std::string, std::string> params;
    if (tokens[i].val == "[") {  // Handle parameters
      do {
        ++i;
        TYPE_CHECK(i, identifier);
        std::string param_name = tokens[i++].val;

        VAL_CHECK(i, "=");
        ++i;

        params[param_name] = tokens[i++].val;

        if ((tokens[i].val != ",") && (tokens[i].val != "]")) {
          printf(
              "Error parsing schema line %s: expected ',' or ']' after "
              "parameter definition\n",
              line.c_str());
          printf("Got %s of type %d instead\n", tokens[i].val.c_str(),
                 (int)(tokens[i].type));
          std::exit(1);
        }

      } while (tokens[i].val == ",");
      ++i;
    }

    VAL_CHECK(i, "(");
    ++i;

    // Handle domains and maybe base emission
    std::string base_relation;
    std::vector<std::string> domains;

    TYPE_CHECK(i, identifier);

    if (tokens[i + 1].val == ";") {  // We have an emission!
      base_relation = tokens[i].val;
      i += 2;
    }

    do {
      domains.push_back(tokens[i++].val);

      if ((tokens[i].val != ",") && (tokens[i].val != ")")) {
        printf(
            "Error parsing schema line %s: expected ',' or ')' after domain\n",
            line.c_str());
        printf("Got %s of type %d instead\n", tokens[i].val.c_str(),
               (int)(tokens[i].type));
        std::exit(1);
      }
    } while (tokens[i++].val == ",");

    if (base_relation.empty()) {  // Clean relation
      T_clean_relation relation;
      relation.domains = domains;
      assert(relation.domains.size() > 0);
      relation.distribution_spec = DistributionSpec(dist_name, params);
      // If the data contains observations of this relation, this bool will be
      // overwritten to true.
      relation.is_observed = false;
      schema[relname] = relation;
    } else {
      T_noisy_relation relation;
      relation.domains = domains;
      assert(relation.domains.size() > 0);
      relation.emission_spec = EmissionSpec(dist_name, params);
      relation.base_relation = base_relation;
      // If the data contains observations of this relation, this bool will be
      // overwritten to true.
      relation.is_observed = false;
      schema[relname] = relation;
    }
  }
  fp.close();
  verify_noisy_relation_domains(schema);
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

    if (!getline(stream, value_str, ',')) {
      assert(false && "Error parsing schema. No comma separating value.");
    }

    if (!getline(stream, relname, ',')) {
      assert(false && "Error parsing schema. No comma separating relation.");
    }

    if (!schema.contains(relname)) {
      printf("Can not find %s in schema\n", relname.c_str());
      std::exit(1);
    }

    std::string word;
    while (getline(stream, word, ',')) {
      items.push_back(word);
    }
    assert((items.size() > 0) && "No Domain values specified.");
    auto entry = std::make_tuple(items, value_str);
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

void write_observations(std::ostream& fp,
                        const T_encoded_observations& observations,
                        const T_encoding& encoding,
                        std::variant<IRM*, HIRM*> h_irm) {
  const T_encoding_r& reverse_encoding = std::get<1>(encoding);
  for (const auto& [rel, obs_for_rel] : observations) {
    T_relation trel =
        std::visit([&](const auto& m) { return m->schema.at(rel); }, h_irm);
    std::vector<std::string> domains =
        std::visit([&](const auto& tr) { return tr.domains; }, trel);
    for (const auto& [entities, value_str] : obs_for_rel) {
      fp << value_str << "," << rel;
      for (size_t i = 0; i < entities.size(); ++i) {
        fp << ",";
        const auto& rev_encoding = reverse_encoding.at(domains[i]);
        auto it = rev_encoding.find(entities[i]);
        if (it == rev_encoding.end()) {
          fp << "new_" << domains[i] << "_" << entities[i];
        } else {
          fp << it->second;
        }
      }
      fp << "\n";
    }
  }
}

void write_observations(const std::string& path,
                        const T_encoded_observations& observations,
                        const T_encoding& encoding,
                        std::variant<IRM*, HIRM*> h_irm) {
  std::ofstream fp(path);
  assert(fp.good());
  write_observations(fp, observations, encoding, h_irm);
  fp.close();
}

// Assumes that T_item is integer.
T_encoding calculate_encoding(const T_schema& schema,
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

T_encoded_observations encode_observations(const T_observations& observations,
                                           const T_encoding& encoding,
                                           std::variant<IRM*, HIRM*> h_irm) {
  T_encoded_observations encoded_observations;
  T_encoding_f item_to_code = std::get<0>(encoding);

  for (const auto& [relation, obs] : observations) {
    T_relation trel = std::visit(
        [&](const auto& m) { return m->schema.at(relation); }, h_irm);
    std::vector<std::string> domains =
        std::visit([&](const auto& tr) { return tr.domains; }, trel);
    for (const auto& [items, value] : obs) {
      T_items items_e;
      for (size_t i = 0; i < items.size(); ++i) {
        int code = item_to_code.at(domains[i]).at(items[i]);
        items_e.push_back(code);
      }
      encoded_observations[relation].push_back(std::make_pair(items_e, value));
    }
  }

  return encoded_observations;
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

  ObservationVariant ov;
  if (observations.contains(relation)) {
    // If this relation is observed, incorporate its observations.
    for (const auto& [items, value] : observations.at(relation)) {
      std::visit([&](const auto& r) { ov = r->from_string(value); }, rel_var);
      std::visit([&](auto& m) { m->incorporate(prng, relation, items, ov); },
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
  T_encoded_observations encoded_observations =
      encode_observations(observations, encoding, h_irm);

  std::unordered_map<std::string, std::string> noisy_to_base;
  std::unordered_map<std::string, std::vector<std::string>> base_to_noisy =
      std::visit([](const auto& m) { return m->base_to_noisy_relations; },
                 h_irm);
  for (const auto& [base_name, noisy_names] : base_to_noisy) {
    for (const std::string& noisy_name : noisy_names) {
      if (!base_to_noisy.contains(noisy_name)) {
        if (!observations.contains(noisy_name)) {
          printf(
              "Relation %s has no observations and is not the base of a noisy "
              "relation.\n",
              noisy_name.c_str());
          std::exit(1);
        }
      }
      noisy_to_base[noisy_name] = base_name;
    }
  }

  std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
      relation_items;
  for (const auto& [relation, obs_for_rel] : encoded_observations) {
    for (const auto& [items, unused_value] : obs_for_rel) {
      relation_items[relation].insert(items);
    }
  }

  std::unordered_set<std::string> completed_relations;
  for (const auto& [relation, unused] : encoded_observations) {
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
      // Iterate in order. Create a list of item names:
      std::set<std::string> item_names;
      for (const T_item& item : items) {
        item_names.insert(code_to_item.at(domain->name).at(item));
      }
      for (const std::string& item_name : item_names) {
        fp << item_name;
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
    // Ignore the new lines that are there for readability.
    if (line.size() == 0) {
      continue;
    }
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
    if (line.size() == 0) {
      // Start of new IRM
      irmc = -1;
      continue;
    }

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
    if (irms.count(t) != 1) {
      assert(false);
    }
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
  T_encoding encoding = calculate_encoding(schema, observations);
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
  T_encoding encoding = calculate_encoding(schema, observations);
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

T_observations merge_observations(const T_observations& obs1,
                                  const T_observations& obs2) {
  T_observations merged;
  for (const auto& it : obs1) {
    merged[it.first] = it.second;
  }
  for (const auto& it : obs2) {
    merged[it.first].insert(merged[it.first].end(), it.second.begin(),
                            it.second.end());
  }
  return merged;
}

double logp(std::mt19937* prng, std::variant<IRM*, HIRM*> h_irm,
            const T_encoding& encoding, const T_observations& observations) {
  T_encoded_observations encoded_obs =
      encode_observations(observations, encoding, h_irm);
  std::vector<std::tuple<std::string, T_items, ObservationVariant>> logp_obs;
  for (const auto& [relation, obs_for_rel] : encoded_obs) {
    RelationVariant rel_var =
        std::visit([&](auto m) { return m->get_relation(relation); }, h_irm);
    for (const auto& [items, value] : obs_for_rel) {
      ObservationVariant ov;
      std::visit([&](const auto& r) { ov = r->from_string(value); }, rel_var);
      logp_obs.push_back(make_tuple(relation, items, ov));
    }
  }
  return std::visit([&](const auto& m) { return m->logp(logp_obs, prng); },
                    h_irm);
}
