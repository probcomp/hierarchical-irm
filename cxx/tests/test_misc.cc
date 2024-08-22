// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hirm.hh"
#include "relation.hh"
#include "util_hash.hh"
#include "util_io.hh"
#include "util_math.hh"

int main(int argc, char** argv) {
  srand(1);
  std::mt19937 prng(1);

  printf("== HASHING UTIL == \n");
  std::unordered_map<const std::vector<int>, int, VectorIntHash> map_int;
  map_int[{1, 2}] = 7;
  printf("%d\n", map_int.at({1, 2}));
  std::unordered_map<const std::vector<std::string>, int, VectorStringHash>
      map_str;
  map_str[{"1", "2", "3"}] = 7;
  printf("%d\n", map_str.at({"1", "2", "3"}));

  printf("===== IRM ====\n");
  std::map<std::string, T_relation> schema1{
      {"R1", T_clean_relation{{"D1", "D1"}, true, DistributionSpec("bernoulli")}},
      {"R2", T_clean_relation{{"D1", "D2"}, true, DistributionSpec("normal")}},
      {"R3", T_clean_relation{{"D3", "D1"}, true, DistributionSpec("bigram")}}};
  IRM irm(schema1);

  for (auto const& kv : irm.domains) {
    printf("%s %s; ", kv.first.c_str(), kv.second->name.c_str());
    for (auto const& r : irm.domain_to_relations.at(kv.first)) {
      printf("%s ", r.c_str());
    }
    printf("\n");
  }
  for (auto const& kv : irm.relations) {
    printf("%s ", kv.first.c_str());
    for (auto const d :
         std::visit([&](auto r) { return r->get_domains(); }, kv.second)) {
      printf("%s ", d->name.c_str());
    }
    printf("\n");
  }

  printf("==== READING IO ===== \n");
  auto schema = load_schema("assets/animals.binary.schema");
  for (auto const& i : schema) {
    printf("relation: %s\n", i.first.c_str());
    printf("domains: ");
    for (auto const& j :
         std::visit([](const auto& r) { return r.domains; }, i.second)) {
      printf("%s ", j.c_str());
    }
    printf("\n");
  }

  IRM irm3(schema);
  auto observations = load_observations("assets/animals.binary.obs", schema);
  auto encoding = calculate_encoding(schema, observations);
  auto item_to_code = std::get<0>(encoding);
  for (const auto& [relation, obs] : observations) {
    for (const auto& [items, value] : obs) {
      printf("incorporating %s ", relation.c_str());
      printf("val=%s ", value.c_str());
      int counter = 0;
      T_items items_code;
      auto rel_domains =
          std::visit([](const auto& r) { return r.domains; }, schema.at(relation));
      for (auto const& item : items) {
        auto domain = rel_domains[counter];
        counter += 1;
        auto code = item_to_code.at(domain).at(item);
        printf("%s(%d) ", item.c_str(), code);
        items_code.push_back(code);
      }
      printf("\n");
      RelationVariant rv = irm3.get_relation(relation);
      ObservationVariant ov;
      std::visit([&](const auto &r) {ov = r->from_string(value);}, rv);
      irm3.incorporate(&prng, relation, items_code, ov);
    }
  }

  printf("About to transition cluster assignments and alphas\n");
  for (int i = 0; i < 4; i++) {
    irm3.transition_cluster_assignments(&prng, {"animal", "feature"});
    irm3.transition_cluster_assignments_all(&prng);
    for (auto const& [d, domain] : irm3.domains) {
      domain->crp.transition_alpha(&prng);
    }
    double x = irm3.logp_score();
    printf("iter %d, score %f\n", i, x);
  }

  std::string path_clusters = "assets/animals.binary.irm";
  to_txt(path_clusters, irm3, encoding);

  auto rel = reinterpret_cast<CleanRelation<bool>*>(
      std::get<Relation<bool>*>(irm3.relations.at("has")));
  auto& enc = std::get<0>(encoding);
  auto lp0 = rel->logp({enc["animal"]["tail"], enc["animal"]["bat"]}, 0, &prng);
  auto lp1 = rel->logp({enc["animal"]["tail"], enc["animal"]["bat"]}, 1, &prng);
  auto lp_01 = logsumexp({lp0, lp1});
  assert(abs(lp_01) < 1e-5);
  printf("log prob of has(tail, bat)=0 is %1.2f\n", lp0);
  printf("log prob of has(tail, bat)=1 is %1.2f\n", lp1);
  printf("logsumexp is %1.2f\n", lp_01);

  IRM irm4({});
  from_txt(&prng, &irm4, "assets/animals.binary.schema",
           "assets/animals.binary.obs", path_clusters);
  irm4.domains.at("animal")->crp.alpha = irm3.domains.at("animal")->crp.alpha;
  irm4.domains.at("feature")->crp.alpha = irm3.domains.at("feature")->crp.alpha;
  assert(abs(irm3.logp_score() - irm4.logp_score()) < 1e-8);
  for (const auto& d : {"animal", "feature"}) {
    [[maybe_unused]] auto d3 = irm3.domains.at(d);
    [[maybe_unused]] auto d4 = irm4.domains.at(d);
    assert(d3->items == d4->items);
    assert(d3->crp.assignments == d4->crp.assignments);
    assert(d3->crp.tables == d4->crp.tables);
    assert(d3->crp.N == d4->crp.N);
    assert(d3->crp.alpha == d4->crp.alpha);
  }
  for (const auto& r : {"has"}) {
    auto r3 = reinterpret_cast<CleanRelation<bool>*>(
        std::get<Relation<bool>*>(irm3.relations.at(r)));
    auto r4 = reinterpret_cast<CleanRelation<bool>*>(
        std::get<Relation<bool>*>(irm4.relations.at(r)));
    assert(r3->data == r4->data);
    assert(r3->data_r == r4->data_r);
    assert(r3->clusters.size() == r4->clusters.size());
    for (const auto& [z, cluster3] : r3->clusters) {
      [[maybe_unused]] auto cluster4 = r4->clusters.at(z);
      assert(cluster3->N == cluster4->N);
    }
  }
}
