// Copyright 2020
// See LICENSE.txt

#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "distributions/get_distribution.hh"
#include "domain.hh"
#include "emissions/get_emission.hh"
#include "relation.hh"
#include "util_hash.hh"
#include "util_math.hh"

// T_clean_relation is the text we get from reading a line of the schema
// file; CleanRelation is the object that does the work.
class T_clean_relation {
 public:
  // The relation is a map from the domains to the space .distribution
  // is a distribution over.
  std::vector<std::string> domains;

  // Indicates if the CleanRelation's values are observed or latent.
  bool is_observed;

  DistributionSpec distribution_spec;
};

template <typename T>
class CleanRelation : public Relation<T> {
 public:
  typedef T ValueType;

  // human-readable name
  const std::string name;
  // list of domain pointers
  const std::vector<Domain*> domains;
  // Distribution or Emission spec over the relation's codomain.
  const std::variant<DistributionSpec, EmissionSpec> prior_spec;
  // map from cluster multi-index to Distribution pointer
  std::unordered_map<const std::vector<int>, Distribution<ValueType>*,
                     VectorIntHash>
      clusters;
  // map from item to observed data
  std::unordered_map<const T_items, ValueType, H_items> data;
  // map from domain name to reverse map from item to
  // set of items that include that item
  std::unordered_map<
      std::string,
      std::unordered_map<T_item, std::unordered_set<T_items, H_items>>>
      data_r;

  CleanRelation(const std::string& name,
                const std::variant<DistributionSpec, EmissionSpec>& prior_spec,
                const std::vector<Domain*>& domains)
      : name(name), domains(domains), prior_spec(prior_spec) {
    assert(!domains.empty());
    assert(!name.empty());
    for (const Domain* const d : domains) {
      this->data_r[d->name] =
          std::unordered_map<T_item, std::unordered_set<T_items, H_items>>();
    }
  }

  ~CleanRelation() {
    for (auto [z, cluster] : clusters) {
      delete cluster;
    }
  }

  Distribution<ValueType>* make_new_distribution(std::mt19937* prng) const {
    auto var_to_dist = [&](auto dist_variant) {
      return std::visit(
          [&](auto v) { return reinterpret_cast<Distribution<ValueType>*>(v); },
          dist_variant);
    };
    auto spec_to_dist = [&](auto spec) {
      return var_to_dist(get_prior(spec, prng));
    };
    return std::visit(spec_to_dist, prior_spec);
  }

  // Incorporates a new vector of items and returns their cluster assignments.
  T_items incorporate_items(std::mt19937* prng, const T_items& items) {
    assert(!data.contains(items));
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
      clusters[z] = make_new_distribution(prng);
    }
    return z;
  }

  void incorporate(std::mt19937* prng, const T_items& items, ValueType value) {
    T_items z = incorporate_items(prng, items);
    data[items] = value;
    clusters.at(z)->incorporate(value);
  }

  void incorporate_sample(std::mt19937* prng, const T_items& items) {
    T_items z = incorporate_items(prng, items);
    ValueType value = sample_at_items(prng, items);
    data[items] = value;
    clusters.at(z)->incorporate(value);
  }

  void unincorporate(const T_items& items) {
    assert(data.contains(items));
    ValueType value = data.at(items);
    std::vector<int> z = get_cluster_assignment(items);
    clusters.at(z)->unincorporate(value);
    if (clusters.at(z)->N == 0) {
      delete clusters.at(z);
      clusters.erase(z);
    }
    for (int i = 0; i < std::ssize(domains); ++i) {
      const std::string& name = domains[i]->name;
      if (data_r.at(name).contains(items[i])) {
        data_r.at(name).at(items[i]).erase(items);
        if (data_r.at(name).at(items[i]).size() == 0) {
          // It's safe to unincorporate this element since no other data point
          // refers to it.
          data_r.at(name).erase(items[i]);
          domains[i]->unincorporate(items[i]);
        }
      }
    }
    data.erase(items);
  }

  // incorporate_to_cluster and unincorporate_from_cluster should be used with
  // extreme care, since they mutate the clusters only and not the relation. In
  // particular, for every call to unincorporate_from_cluster, there must be a
  // corresponding call to incorporate_to_cluster with the same items, or the
  // Relation will be in an invalid state. See `transition_latent_value` for
  // usage/justification of this choice.
  void incorporate_to_cluster(const T_items& items, const ValueType& value) {
    T_items z = get_cluster_assignment(items);
    assert(clusters.contains(z));
    clusters.at(z)->incorporate(value);
  }

  void unincorporate_from_cluster(const T_items& items) {
    T_items z = get_cluster_assignment(items);
    assert(clusters.contains(z));

    const ValueType& value = get_value(items);
    clusters.at(z)->unincorporate(value);
  }

  std::vector<int> get_cluster_assignment(const T_items& items) const {
    assert(items.size() == domains.size());
    std::vector<int> z(domains.size());
    for (int i = 0; i < std::ssize(domains); ++i) {
      z[i] = domains[i]->get_cluster_assignment(items[i]);
    }
    return z;
  }

  double cluster_or_prior_logp(std::mt19937* prng, const T_items& items,
                               const ValueType& value) const {
    if (clusters.contains(items)) {
      return clusters.at(items)->logp(value);
    }
    Distribution<ValueType>* prior = make_new_distribution(prng);
    double prior_logp = prior->logp(value);
    delete prior;
    return prior_logp;
  }

  ValueType sample_at_items(std::mt19937* prng, const T_items& items) const {
    if (clusters.contains(items)) {
      return clusters.at(items)->sample(prng);
    }
    Distribution<ValueType>* prior = make_new_distribution(prng);
    ValueType prior_sample = prior->sample(prng);
    delete prior;
    return prior_sample;
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
                                   int table, std::mt19937* prng) {
    double logp = 0.;
    for (const T_items& items : data_r.at(domain.name).at(item)) {
      ValueType x = data.at(items);
      T_items z = get_cluster_assignment_gibbs(items, domain, item, table);
      double lp;
      if (!clusters.contains(z)) {
        Distribution<ValueType>* tmp_dist = make_new_distribution(prng);
        lp = tmp_dist->logp(x);
        delete tmp_dist;
      } else {
        lp = clusters.at(z)->logp(x);
      }
      logp += lp;
    }
    return logp;
  }

  double logp_gibbs_approx(const Domain& domain, const T_item& item, int table,
                           std::mt19937* prng) {
    int table_current = domain.get_cluster_assignment(item);
    return table_current == table
               ? logp_gibbs_approx_current(domain, item)
               : logp_gibbs_approx_variant(domain, item, table, prng);
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
    // Approximate floating point equality.
    assert(abs(cluster->logp_score() - logp0) <=
           std::numeric_limits<double>::epsilon() * abs(logp0));
    return logp0 - logp1;
  }

  double logp_gibbs_exact_variant(const Domain& domain, const T_item& item,
                                  int table,
                                  const std::vector<T_items>& items_list,
                                  std::mt19937* prng) {
    assert(!items_list.empty());
    T_items z =
        get_cluster_assignment_gibbs(items_list[0], domain, item, table);

    Distribution<ValueType>* prior = make_new_distribution(prng);
    Distribution<ValueType>* cluster =
        clusters.contains(z) ? clusters.at(z) : prior;
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

    // Approximate floating point equality.
    assert(abs(cluster->logp_score() - logp0) <=
           std::numeric_limits<double>::epsilon() * abs(logp0));
    delete prior;
    return logp1 - logp0;
  }

  std::vector<double> logp_gibbs_exact(const Domain& domain, const T_item& item,
                                       std::vector<int> tables,
                                       std::mt19937* prng) {
    auto cluster_to_items_list = get_cluster_to_items_list(domain, item);
    int table_current = domain.get_cluster_assignment(item);
    std::vector<double> logps;
    logps.reserve(tables.size());
    double lp_cluster;
    for (const int& table : tables) {
      double lp_table = 0;
      for (const auto& [z, items_list] : cluster_to_items_list) {
        lp_cluster = (table == table_current)
                         ? logp_gibbs_exact_current(items_list)
                         : logp_gibbs_exact_variant(domain, item, table,
                                                    items_list, prng);
        lp_table += lp_cluster;
      }
      logps.push_back(lp_table);
    }
    return logps;
  }

  double logp(const T_items& items, ValueType value, std::mt19937* prng) {
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
      Distribution<ValueType>* prior = make_new_distribution(prng);
      Distribution<ValueType>* cluster =
          clusters.contains(z) ? clusters.at(z) : prior;
      double logp_z = cluster->logp(value);
      double logp_zw = logp_z + logp_w;
      logps.push_back(logp_zw);
      delete prior;
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
                                    int table, std::mt19937* prng) {
    [[maybe_unused]] int table_current = domain.get_cluster_assignment(item);
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
        clusters[z_new] = make_new_distribution(prng);
        clusters.at(z_new)->incorporate(x);
      } else {
        // Move to existing cluster.
        assert((clusters.at(z_new)->N > 0));
        clusters.at(z_new)->incorporate(x);
      }
    }
    // Caller should invoke domain.set_cluster_gibbs
  }

  bool has_observation(const Domain& domain, const T_item& item) const {
    return data_r.at(domain.name).contains(item);
  }

  const std::vector<Domain*>& get_domains() const { return domains; }

  const ValueType& get_value(const T_items& items) const {
    assert(data.contains(items));
    return data.at(items);
  }

  void update_value(const T_items& items, const ValueType& value) {
    unincorporate_from_cluster(items);
    incorporate_to_cluster(items, value);
    data.at(items) = value;
  }

  const std::unordered_map<const T_items, ValueType, H_items>& get_data()
      const {
    return data;
  }

  void transition_cluster_hparams(std::mt19937* prng, int num_theta_steps) {
    for (const auto& [c, distribution] : clusters) {
      for (int i = 0; i < num_theta_steps; ++i) {
        distribution->transition_theta(prng);
      }
      distribution->transition_hyperparameters(prng);
    }
  }

  // Disable copying.
  CleanRelation& operator=(const CleanRelation&) = delete;
  CleanRelation(const CleanRelation&) = delete;
};
