// Copyright 2020
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "distributions/base.hh"
#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/crp.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"
#include "domain.hh"
#include "util_distribution_variant.hh"
#include "util_hash.hh"
#include "util_math.hh"

typedef std::vector<T_item> T_items;
typedef VectorIntHash H_items;

// T_relation is the text we get from reading a line of the schema file;
// hirm.hh:Relation is the object that does the work.
class T_relation {
 public:
  // The relation is a map from the domains to the space .distribution
  // is a distribution over.
  std::vector<std::string> domains;

  DistributionSpec distribution_spec;
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
  // Relation spec.
  T_relation trel;
  // Distribution spec over the relation's codomain.
  const DistributionSpec dist_spec;
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

  Relation(const std::string& name, const DistributionSpec& dist_spec,
           const std::vector<Domain*>& domains)
      : name(name), dist_spec(dist_spec), domains(domains) {
    assert(!domains.empty());
    assert(!name.empty());
    for (const Domain* const d : domains) {
      this->data_r[d->name] =
          std::unordered_map<T_item, std::unordered_set<T_items, H_items>>();
    }
    std::vector<std::string> domain_names;
    for (const auto& d : domains) {
      domain_names.push_back(d->name);
    }
    trel = {domain_names, dist_spec};
  }

  ~Relation() {
    for (auto [z, cluster] : clusters) {
      delete cluster;
    }
  }

  void incorporate(std::mt19937* prng, const T_items& items, ValueType value) {
    assert(!data.contains(items));
    data[items] = value;
    for (int i = 0; i < std::ssize(domains); ++i) {
      domains[i]->incorporate(prng, items[i]);
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
      clusters[z] =
          std::get<DistributionType*>(cluster_prior_from_spec(dist_spec));
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
        DistributionType* cluster =
            std::get<DistributionType*>(cluster_prior_from_spec(dist_spec));
        lp = cluster->logp(x);
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

    DistributionType* prior =
        std::get<DistributionType*>(cluster_prior_from_spec(dist_spec));
    DistributionType* cluster = clusters.contains(z) ? clusters.at(z) : prior;
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
    std::vector<double> logps;
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
      DistributionType* prior =
          std::get<DistributionType*>(cluster_prior_from_spec(dist_spec));
      DistributionType* cluster = clusters.contains(z) ? clusters.at(z) : prior;
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
        clusters[z_new] =
            std::get<DistributionType*>(cluster_prior_from_spec(dist_spec));
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
