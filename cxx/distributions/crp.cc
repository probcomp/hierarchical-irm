// Copyright 2024
// See LICENSE.txt

#include "distributions/crp.hh"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "util_math.hh"

void CRP::incorporate(const T_item& item, int table) {
  assert(!assignments.contains(item));
  if (!tables.contains(table)) {
    tables[table] = std::unordered_set<T_item>();
  }
  tables.at(table).insert(item);
  assignments[item] = table;
  ++N;
}

void CRP::unincorporate(const T_item& item) {
  assert(assignments.contains(item));
  int table = assignments.at(item);
  tables.at(table).erase(item);
  if (tables.at(table).empty()) {
    tables.erase(table);
  }
  assignments.erase(item);
  --N;
}

int CRP::sample(std::mt19937* prng) {
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

double CRP::logp_new_table() const { return log(alpha) - log(N + alpha); }

double CRP::logp(int table) const {
  auto dist = tables_weights();
  if (!dist.contains(table)) {
    return -std::numeric_limits<double>::infinity();
  }
  double numer = dist[table];
  double denom = N + alpha;
  return log(numer) - log(denom);
}

double CRP::logp_score() const {
  double term1 = tables.size() * log(alpha);
  double term2 = 0;
  for (const auto& [table, customers] : tables) {
    term2 += lgamma(customers.size());
  }
  double term3 = lgamma(alpha);
  double term4 = lgamma(N + alpha);
  double out = term1 + term2 + term3 - term4;
  return out;
}

int CRP::max_table() const {
  if (N == 0) {
    return 0;
  }
  return tables.rbegin()->first;
}

std::unordered_map<int, double> CRP::tables_weights() const {
  std::unordered_map<int, double> dist;
  if (N == 0) {
    dist[0] = 1;
    return dist;
  }
  for (const auto& [table, customers] : tables) {
    dist[table] = customers.size();
  }
  dist[max_table() + 1] = alpha;
  return dist;
}

std::unordered_map<int, double> CRP::tables_weights_gibbs(int table) const {
  assert(N > 0);
  assert(tables.contains(table));
  auto dist = tables_weights();
  --dist.at(table);
  if (dist.at(table) == 0) {
    dist.at(table) = alpha;
    dist.erase(max_table());
  }
  return dist;
}

void CRP::transition_alpha(std::mt19937* prng) {
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
