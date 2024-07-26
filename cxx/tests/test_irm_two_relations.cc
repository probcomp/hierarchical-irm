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
#include <vector>

#include "irm.hh"
#include "util_io.hh"
#include "util_math.hh"
#include "distributions/beta_bernoulli.hh"

using T_r = CleanRelation<bool>*;

int main(int argc, char** argv) {
  std::string path_base = "assets/two_relations";
  int seed = 1;
  int iters = 2;

  std::mt19937 prng(seed);

  std::string path_schema = path_base + ".schema";
  std::cout << "loading schema from " << path_schema << std::endl;
  auto schema = load_schema(path_schema);
  for (auto const& [relation_name, relation] : schema) {
    printf("relation: %s, ", relation_name.c_str());
    printf("domains: ");
    for (auto const& domain : std::visit([](auto r) {return r.domains;}, relation)) {
      printf("%s ", domain.c_str());
    }
    printf("\n");
  }

  std::string path_obs = path_base + ".obs";
  std::cout << "loading observations from " << path_obs << std::endl;
  auto observations = load_observations(path_obs, schema);
  T_encoding encoding = encode_observations(schema, observations);

  IRM irm(schema);
  incorporate_observations(&prng, &irm, encoding, observations);
  printf("running for %d iterations\n", iters);
  double t_total = 0.0;
  for (int i = 0; i < iters; i++) {
    single_step_irm_inference(&prng, &irm, t_total, true);
    printf("iter %d, score %f\n", i, irm.logp_score());
  }

  std::string path_clusters = path_base + ".irm";
  std::cout << "writing clusters to " << path_clusters << std::endl;
  to_txt(path_clusters, irm, encoding);

  auto item_to_code = std::get<0>(encoding);
  auto code_item_0_D1 = item_to_code.at("D1").at("0");
  auto code_item_10_D1 = item_to_code.at("D1").at("10");
  auto code_item_0_D2 = item_to_code.at("D2").at("0");
  auto code_item_10_D2 = item_to_code.at("D2").at("10");
  auto code_item_novel = 100;

  std::map<int, std::map<int, double>> expected_p0{
      {code_item_0_D1,
       {{code_item_0_D2, 1}, {code_item_10_D2, 1}, {code_item_novel, .5}}},
      {code_item_10_D1,
       {{code_item_0_D2, 0}, {code_item_10_D2, 0}, {code_item_novel, .5}}},
      {code_item_novel,
       {{code_item_0_D2, .66}, {code_item_10_D2, .66}, {code_item_novel, .5}}},
  };

  std::vector<std::vector<int>> indexes{
      {code_item_0_D1, code_item_10_D1, code_item_novel},
      {code_item_0_D1, code_item_10_D2, code_item_novel}};
  for (const auto& l : product(indexes)) {
    assert(l.size() == 2);
    auto x1 = l.at(0);
    auto x2 = l.at(1);
    auto p0 =
        reinterpret_cast<T_r>(std::get<Relation<bool>*>(irm.relations.at("R1")))
            ->logp({x1, x2}, false, &prng);
    auto p0_irm = irm.logp({{"R1", {x1, x2}, false}}, &prng);
    assert(abs(p0 - p0_irm) < 1e-10);
    auto p1 =
        reinterpret_cast<T_r>(std::get<Relation<bool>*>(irm.relations.at("R1")))
            ->logp({x1, x2}, true, &prng);
    auto Z = logsumexp({p0, p1});
    assert(abs(Z) < 1e-10);
    assert(abs(exp(p0) - expected_p0[x1].at(x2)) < .1);
  }

  for (const auto& l :
       std::vector<std::vector<int>>{{0, 10, 100}, {110, 10, 100}}) {
    auto x1 = l.at(0);
    auto x2 = l.at(1);
    auto x3 = l.at(2);
    auto p00 =
        irm.logp({{"R1", {x1, x2}, false}, {"R1", {x1, x3}, false}}, &prng);
    auto p01 =
        irm.logp({{"R1", {x1, x2}, false}, {"R1", {x1, x3}, true}}, &prng);
    auto p10 =
        irm.logp({{"R1", {x1, x2}, true}, {"R1", {x1, x3}, false}}, &prng);
    auto p11 =
        irm.logp({{"R1", {x1, x2}, true}, {"R1", {x1, x3}, true}}, &prng);
    auto Z = logsumexp({p00, p01, p10, p11});
    assert(abs(Z) < 1e-10);
  }

  IRM irx({});
  from_txt(&prng, &irx, path_schema, path_obs, path_clusters);
  // Check log scores agree.
  for (const auto& d : {"D1", "D2"}) {
    auto dm = irm.domains.at(d);
    auto dx = irx.domains.at(d);
    dx->crp.alpha = dm->crp.alpha;
  }
  // They shouldn't agree yet because irx's hyperparameters haven't been
  // transitioned.
  assert(abs(irx.logp_score() - irm.logp_score()) > 1e-8);
  for (const auto& r : {"R1", "R2"}) {
    auto r1m =
        reinterpret_cast<T_r>(std::get<Relation<bool>*>(irm.relations.at(r)));
    auto r1x =
        reinterpret_cast<T_r>(std::get<Relation<bool>*>(irx.relations.at(r)));
    for (const auto& [c, distribution] : r1m->clusters) {
      auto dx = reinterpret_cast<BetaBernoulli*>(r1x->clusters.at(c));
      auto dy = reinterpret_cast<BetaBernoulli*>(distribution);
      dx->alpha = dy->alpha;
      dx->beta = dy->beta;
    }
  }
  assert(abs(irx.logp_score() - irm.logp_score()) < 1e-8);
  // Check domains agree.
  for (const auto& d : {"D1", "D2"}) {
    auto dm = irm.domains.at(d);
    auto dx = irx.domains.at(d);
    assert(dm->items == dx->items);
    assert(dm->crp.assignments == dx->crp.assignments);
    assert(dm->crp.tables == dx->crp.tables);
    assert(dm->crp.N == dx->crp.N);
    assert(dm->crp.alpha == dx->crp.alpha);
  }
  // Check relations agree.
  for (const auto& r : {"R1", "R2"}) {
    auto rm_var = irm.relations.at(r);
    auto rx_var = irx.relations.at(r);
    T_r rm = reinterpret_cast<T_r>(std::get<Relation<bool>*>(rm_var));
    T_r rx = reinterpret_cast<T_r>(std::get<Relation<bool>*>(rx_var));
    assert(rm->data == rx->data);
    assert(rm->data_r == rx->data_r);
    assert(rm->clusters.size() == rx->clusters.size());
    for (const auto& [z, clusterm] : rm->clusters) {
      auto clusterx = rx->clusters.at(z);
      assert(clusterm->N == clusterx->N);
    }
  }
}
