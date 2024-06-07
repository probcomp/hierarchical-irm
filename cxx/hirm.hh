// Copyright 2020
// See LICENSE.txt

#pragma once
#include <functional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "distributions/base.hh"
#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "util_hash.hh"
#include "util_math.hh"

typedef int T_item;
typedef std::vector<T_item> T_items;
typedef VectorIntHash H_items;

// T_relation is the text we get from reading a line of the schema file;
// hirm.hh:Relation is the object that does the work.
class T_relation {
 public:
  // The relation is a map from the domains to the space .distribution
  // is a distribution over.
  std::vector<std::string> domains;

  // Must be the name of a distribution in distributions/.
  std::string distribution;
};

// Map from names to T_relation's.
typedef std::map<std::string, T_relation> T_schema;

using ObservationVariant = std::variant<double, int, std::string>;

// TODO(emilyaf): Make this a distribution subclass.
class CRP {
 public:
  double alpha = 1;  // concentration parameter
  int N = 0;         // number of customers
  std::unordered_map<int, std::unordered_set<T_item>>
      tables;  // map from table id to set of customers
  std::unordered_map<T_item, int> assignments;  // map from customer to table id
  std::mt19937* prng;

  CRP(std::mt19937* prng) { this->prng = prng; }

  void incorporate(const T_item& item, int table) {
    assert(!assignments.contains(item));
    if (!tables.contains(table)) {
      tables[table] = std::unordered_set<T_item>();
    }
    tables.at(table).insert(item);
    assignments[item] = table;
    ++N;
  }
  void unincorporate(const T_item& item) {
    assert(assignments.contains(item));
    int table = assignments.at(item);
    tables.at(table).erase(item);
    if (tables.at(table).empty()) {
      tables.erase(table);
    }
    assignments.erase(item);
    --N;
  }
  int sample() {
    auto crp_dist = tables_weights();
    std::vector<int> items(crp_dist.size());
    std::vector<double> weights(crp_dist.size());
    int i = 0;
    for (const auto& [table, weight] : crp_dist) {
      items[i] = table;
      weights[i] = weight;
      ++i;
    }
    int idx = choice(weights, prng);
    return items[idx];
  }
  double logp(int table) const {
    auto dist = tables_weights();
    if (!dist.contains(table)) {
      return -std::numeric_limits<double>::infinity();
    }
    double numer = dist[table];
    double denom = N + alpha;
    return log(numer) - log(denom);
  }
  double logp_score() const {
    double term1 = tables.size() * log(alpha);
    double term2 = 0;
    for (const auto& [table, customers] : tables) {
      term2 += lgamma(customers.size());
    }
    double term3 = lgamma(alpha);
    double term4 = lgamma(N + alpha);
    return term1 + term2 + term3 - term4;
  }
  std::unordered_map<int, double> tables_weights() const {
    std::unordered_map<int, double> dist;
    if (N == 0) {
      dist[0] = 1;
      return dist;
    }
    int t_max = 0;
    for (const auto& [table, customers] : tables) {
      dist[table] = customers.size();
      t_max = std::max(table, t_max);
    }
    dist[t_max + 1] = alpha;
    return dist;
  }
  std::unordered_map<int, double> tables_weights_gibbs(int table) const {
    assert(N > 0);
    assert(tables.contains(table));
    auto dist = tables_weights();
    --dist.at(table);
    if (dist.at(table) == 0) {
      dist.at(table) = alpha;
      int t_max = 0;
      for (const auto& [table, weight] : dist) {
        t_max = std::max(table, t_max);
      }
      dist.erase(t_max);
    }
    return dist;
  }
  void transition_alpha() {
    if (N == 0) {
      return;
    }
    std::vector<double> grid = log_linspace(1. / N, N + 1, 20, true);
    std::vector<double> logps;
    for (const double& g : grid) {
      this->alpha = g;
      double logp_g = logp_score();
      logps.push_back(logp_g);
    }
    int idx = log_choice(logps, prng);
    this->alpha = grid[idx];
  }
};

class Domain {
 public:
  const std::string name;            // human-readable name
  std::unordered_set<T_item> items;  // set of items
  CRP crp;                           // clustering model for items
  std::mt19937* prng;

  Domain(const std::string& name, std::mt19937* prng) : name(name), crp(prng) {
    assert(!name.empty());
    this->prng = prng;
  }
  void incorporate(const T_item& item, int table = -1) {
    if (items.contains(item)) {
      assert(table == -1);
    } else {
      items.insert(item);
      int t = 0 <= table ? table : crp.sample();
      crp.incorporate(item, t);
    }
  }
  void unincorporate(const T_item& item) {
    printf("Not implemented\n");
    exit(EXIT_FAILURE);
    // assert(items.count(item) == 1);
    // assert(items.at(item).count(relation) == 1);
    // items.at(item).erase(relation);
    // if (items.at(item).size() == 0) {
    //     crp.unincorporate(item);
    //     items.erase(item);
    // }
  }
  int get_cluster_assignment(const T_item& item) const {
    assert(items.contains(item));
    return crp.assignments.at(item);
  }
  void set_cluster_assignment_gibbs(const T_item& item, int table) {
    assert(items.contains(item));
    assert(crp.assignments.at(item) != table);
    crp.unincorporate(item);
    crp.incorporate(item, table);
  }
  std::unordered_map<int, double> tables_weights() const {
    return crp.tables_weights();
  }
  std::unordered_map<int, double> tables_weights_gibbs(
      const T_item& item) const {
    int table = get_cluster_assignment(item);
    return crp.tables_weights_gibbs(table);
  }
};

template <typename DistributionType>
class Relation {
 public:
  using ValueType = typename DistributionType::SampleType;
  using DType = DistributionType;
  static_assert(std::is_base_of<Distribution<ValueType>, DType>::value, 
                "DistributionType must inherit from Distribution.");
  // human-readable name
  const std::string name;
  // Distribution over the relation's codomain.
  const std::string distribution;
  // list of domain pointers
  const std::vector<Domain*> domains;
  // map from cluster multi-index to Distribution pointer
  std::unordered_map<const std::vector<int>, DistributionType*, VectorIntHash>
      clusters;
  // map from item to observed data
  std::unordered_map<const T_items, ValueType, H_items> data;
  // map from domain name to reverse map from item to
  // set of items that include that item
  std::unordered_map<
      std::string,
      std::unordered_map<T_item, std::unordered_set<T_items, H_items>>>
      data_r;
  std::mt19937* prng;

  Relation(const std::string& name, const std::string& distribution,
           const std::vector<Domain*>& domains, std::mt19937* prng)
      : name(name), distribution(distribution), domains(domains) {
    assert(!domains.empty());
    assert(!name.empty());
    this->prng = prng;
    for (const Domain* const d : domains) {
      this->data_r[d->name] =
          std::unordered_map<T_item, std::unordered_set<T_items, H_items>>();
    }
  }

  ~Relation() {
    for (auto [z, cluster] : clusters) {
      delete cluster;
    }
  }

  T_relation get_T_relation() {
    T_relation trel;
    trel.distribution = distribution;
    for (const auto& d : domains) {
      trel.domains.push_back(d->name);
    }
    return trel;
  }

  void incorporate(const T_items& items, ValueType value) {
    assert(!data.contains(items));
    data[items] = value;
    for (int i = 0; i < std::ssize(domains); ++i) {
      domains[i]->incorporate(items[i]);
      if (!data_r.at(domains[i]->name).contains(items[i])) {
        data_r.at(domains[i]->name)[items[i]] =
            std::unordered_set<T_items, H_items>();
      }
      data_r.at(domains[i]->name).at(items[i]).insert(items);
    }
    T_items z = get_cluster_assignment(items);
    if (!clusters.contains(z)) {
      // Invalid discussion as using pointers now;
      //      Cannot use clusters[z] because BetaBernoulli
      //      does not have a default constructor, whereas operator[]
      //      calls default constructor when the key does not exist.
      clusters[z] = new DistributionType(prng);
    }
    clusters.at(z)->incorporate(value);
  }

  void unincorporate(const T_items& items) {
    printf("Not implemented\n");
    exit(EXIT_FAILURE);
    // auto x = data.at(items);
    // auto z = get_cluster_assignment(items);
    // clusters.at(z)->unincorporate(x);
    // if (clusters.at(z)->N == 0) {
    //     delete clusters.at(z);
    //     clusters.erase(z);
    // }
    // for (int i = 0; i < domains.size(); i++) {
    //     const std::string &n = domains[i]->name;
    //     if (data_r.at(n).count(items[i]) > 0) {
    //         data_r.at(n).at(items[i]).erase(items);
    //         if (data_r.at(n).at(items[i]).size() == 0) {
    //             data_r.at(n).erase(items[i]);
    //             domains[i]->unincorporate(name, items[i]);
    //         }
    //     }
    // }
    // data.erase(items);
  }

  std::vector<int> get_cluster_assignment(const T_items& items) const {
    assert(items.size() == domains.size());
    std::vector<int> z(domains.size());
    for (int i = 0; i < std::ssize(domains); ++i) {
      z[i] = domains[i]->get_cluster_assignment(items[i]);
    }
    return z;
  }

  std::vector<int> get_cluster_assignment_gibbs(const T_items& items,
                                                const Domain& domain,
                                                const T_item& item,
                                                int table) const {
    assert(items.size() == domains.size());
    std::vector<int> z(domains.size());
    int hits = 0;
    for (int i = 0; i < std::ssize(domains); ++i) {
      if ((domains[i]->name == domain.name) && (items[i] == item)) {
        z[i] = table;
        ++hits;
      } else {
        z[i] = domains[i]->get_cluster_assignment(items[i]);
      }
    }
    assert(hits > 0);
    return z;
  }

  // Implementation of approximate Gibbs data probabilities (faster).

  double logp_gibbs_approx_current(const Domain& domain, const T_item& item) {
    double logp = 0.;
    for (const T_items& items : data_r.at(domain.name).at(item)) {
      ValueType x = data.at(items);
      T_items z = get_cluster_assignment(items);
      auto cluster = clusters.at(z);
      cluster->unincorporate(x);
      double lp = cluster->logp(x);
      cluster->incorporate(x);
      logp += lp;
    }
    return logp;
  }

  double logp_gibbs_approx_variant(const Domain& domain, const T_item& item,
                                   int table) {
    double logp = 0.;
    for (const T_items& items : data_r.at(domain.name).at(item)) {
      ValueType x = data.at(items);
      T_items z = get_cluster_assignment_gibbs(items, domain, item, table);
      double lp;
      if (!clusters.contains(z)) {
        DistributionType cluster(prng);
        lp = cluster.logp(x);
      } else {
        lp = clusters.at(z)->logp(x);
      }
      logp += lp;
    }
    return logp;
  }

  double logp_gibbs_approx(const Domain& domain, const T_item& item,
                           int table) {
    int table_current = domain.get_cluster_assignment(item);
    return table_current == table
               ? logp_gibbs_approx_current(domain, item)
               : logp_gibbs_approx_variant(domain, item, table);
  }

  // Implementation of exact Gibbs data probabilities.

  std::unordered_map<std::vector<int> const, std::vector<T_items>,
                     VectorIntHash>
  get_cluster_to_items_list(Domain const& domain, const T_item& item) {
    std::unordered_map<const std::vector<int>, std::vector<T_items>,
                       VectorIntHash>
        m;
    for (const T_items& items : data_r.at(domain.name).at(item)) {
      T_items z = get_cluster_assignment(items);
      m[z].push_back(items);
    }
    return m;
  }

  double logp_gibbs_exact_current(const std::vector<T_items>& items_list) {
    assert(!items_list.empty());
    T_items z = get_cluster_assignment(items_list[0]);
    auto cluster = clusters.at(z);
    double logp0 = cluster->logp_score();
    for (const T_items& items : items_list) {
      ValueType x = data.at(items);
      // assert(z == get_cluster_assignment(items));
      cluster->unincorporate(x);
    }
    double logp1 = cluster->logp_score();
    for (const T_items& items : items_list) {
      ValueType x = data.at(items);
      cluster->incorporate(x);
    }
    assert(cluster->logp_score() == logp0);
    return logp0 - logp1;
  }

  double logp_gibbs_exact_variant(const Domain& domain, const T_item& item,
                                  int table,
                                  const std::vector<T_items>& items_list) {
    assert(!items_list.empty());
    T_items z =
        get_cluster_assignment_gibbs(items_list[0], domain, item, table);

    DistributionType aux(prng);
    DistributionType* cluster = clusters.contains(z) ? clusters.at(z) : &aux;
    // auto cluster = self.clusters.get(z, self.aux())
    double logp0 = cluster->logp_score();
    for (const T_items& items : items_list) {
      // assert(z == get_cluster_assignment_gibbs(items, domain, item, table));
      ValueType x = data.at(items);
      cluster->incorporate(x);
    }
    const double logp1 = cluster->logp_score();
    for (const T_items& items : items_list) {
      ValueType x = data.at(items);
      cluster->unincorporate(x);
    }
    assert(cluster->logp_score() == logp0);
    return logp1 - logp0;
  }

  std::vector<double> logp_gibbs_exact(const Domain& domain, const T_item& item,
                                       std::vector<int> tables) {
    auto cluster_to_items_list = get_cluster_to_items_list(domain, item);
    int table_current = domain.get_cluster_assignment(item);
    std::vector<double> logps;  // size this?
    logps.reserve(tables.size());
    double lp_cluster;
    for (const int& table : tables) {
      double lp_table = 0;
      for (const auto& [z, items_list] : cluster_to_items_list) {
        lp_cluster =
            (table == table_current)
                ? logp_gibbs_exact_current(items_list)
                : logp_gibbs_exact_variant(domain, item, table, items_list);
        lp_table += lp_cluster;
      }
      logps.push_back(lp_table);
    }
    return logps;
  }

  double logp(const T_items& items, ValueType value) {
    // TODO: Falsely assumes cluster assignments of items
    // from same domain are identical, see note in hirm.py
    assert(items.size() == domains.size());
    std::vector<std::vector<T_item>> tabl_list;
    std::vector<std::vector<double>> wght_list;
    std::vector<std::vector<int>> indx_list;
    for (int i = 0; i < std::ssize(domains); ++i) {
      Domain* domain = domains.at(i);
      T_item item = items.at(i);
      std::vector<T_item> t_list;
      std::vector<double> w_list;
      std::vector<int> i_list;
      if (domain->items.contains(item)) {
        int z = domain->get_cluster_assignment(item);
        t_list = {z};
        w_list = {0};
        i_list = {0};
      } else {
        auto tables_weights = domain->tables_weights();
        double Z = log(domain->crp.alpha + domain->crp.N);
        int idx = 0;
        for (const auto& [t, w] : tables_weights) {
          t_list.push_back(t);
          w_list.push_back(log(w) - Z);
          i_list.push_back(idx++);
        }
        assert(idx == std::ssize(t_list));
      }
      tabl_list.push_back(t_list);
      wght_list.push_back(w_list);
      indx_list.push_back(i_list);
    }
    std::vector<double> logps;
    for (const auto& indexes : product(indx_list)) {
      assert(indexes.size() == domains.size());
      std::vector<int> z;
      z.reserve(domains.size());
      double logp_w = 0;
      for (int i = 0; i < std::ssize(domains); ++i) {
        T_item zi = tabl_list.at(i).at(indexes[i]);
        double wi = wght_list.at(i).at(indexes[i]);
        z.push_back(zi);
        logp_w += wi;
      }
      DistributionType aux(prng);
      DistributionType* cluster = clusters.contains(z) ? clusters.at(z) : &aux;
      double logp_z = cluster->logp(value);
      double logp_zw = logp_z + logp_w;
      logps.push_back(logp_zw);
    }
    return logsumexp(logps);
  }

  double logp_score() const {
    double logp = 0.0;
    for (const auto& [_, cluster] : clusters) {
      logp += cluster->logp_score();
    }
    return logp;
  }

  void set_cluster_assignment_gibbs(const Domain& domain, const T_item& item,
                                    int table) {
    int table_current = domain.get_cluster_assignment(item);
    assert(table != table_current);
    for (const T_items& items : data_r.at(domain.name).at(item)) {
      ValueType x = data.at(items);
      // Remove from current cluster.
      T_items z_prev = get_cluster_assignment(items);
      auto cluster_prev = clusters.at(z_prev);
      cluster_prev->unincorporate(x);
      if (cluster_prev->N == 0) {
        delete clusters.at(z_prev);
        clusters.erase(z_prev);
      }
      // Move to desired cluster.
      T_items z_new = get_cluster_assignment_gibbs(items, domain, item, table);
      if (!clusters.contains(z_new)) {
        // Move to fresh cluster.
        clusters[z_new] = new DistributionType(prng);
        clusters.at(z_new)->incorporate(x);
      } else {
        // Move to existing cluster.
        assert((clusters.at(z_new)->N > 0));
        clusters.at(z_new)->incorporate(x);
      }
    }
    // Caller should invoke domain.set_cluster_gibbs
  }

  bool has_observation(const Domain& domain, const T_item& item) {
    return data_r.at(domain.name).contains(item);
  }

  // Disable copying.
  Relation& operator=(const Relation&) = delete;
  Relation(const Relation&) = delete;
};

using RelationVariant =
    std::variant<Relation<BetaBernoulli>*, Relation<Bigram>*,
                 // Relation<DirichletCategorical>*,
                 Relation<Normal>*>;

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
        typename std::remove_reference_t<decltype(*rel)>::DType aux(prng);
        auto cluster = rel->clusters.contains(z) ? rel->clusters.at(z) : &aux;
        auto v = std::get<
            typename std::remove_reference_t<decltype(*rel)>::ValueType>(value);
        return cluster->logp(v);
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
    if (relation.distribution == "normal") {
      relations[name] =
          new Relation<Normal>(name, relation.distribution, doms, prng);
    } else if (relation.distribution == "bernoulli") {
      relations[name] =
          new Relation<BetaBernoulli>(name, relation.distribution, doms, prng);
    } else if (relation.distribution == "bigram") {
      relations[name] =
          new Relation<Bigram>(name, relation.distribution, doms, prng);
    } else {
      assert(false);
    }
    // TODO(emilyaf): Enable Categorical. Maybe have "categorical5" e.g. for 5
    // categories. relations[name] = new Relation<DirichletCategorical>(name,
    // relation.distribution, doms, prng);
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
        std::visit([](auto rel) { return rel->get_T_relation(); }, relation);
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
      T_relation trel = rel->get_T_relation();
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
