// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include "hirm.hh"

#include <cstdio>
#include <ctime>
#include <iostream>

#include "cxxopts.hpp"
#include "util_io.hh"

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
                               bool verbose) {
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
          for (const auto& [c, distribution] : r->clusters) {
            clock_t t = clock();
            distribution->transition_hyperparameters(prng);
            REPORT_SCORE(verbose, t, t_total, irm);
          }
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

void inference_irm(std::mt19937* prng, IRM* irm, int iters, int timeout,
                   bool verbose) {
  clock_t t_begin = clock();
  double t_total = 0;
  for (int i = 0; i < iters; ++i) {
    CHECK_TIMEOUT(timeout, t_begin);
    single_step_irm_inference(prng, irm, t_total, verbose);
  }
}

void inference_hirm(std::mt19937* prng, HIRM* hirm, int iters, int timeout,
                    bool verbose) {
  clock_t t_begin = clock();
  double t_total = 0;
  for (int i = 0; i < iters; ++i) {
    CHECK_TIMEOUT(timeout, t_begin);
    // TRANSITION RELATIONS.
    for (const auto& [r, rc] : hirm->relation_to_code) {
      clock_t t = clock();
      hirm->transition_cluster_assignment_relation(prng, r);
      REPORT_SCORE(verbose, t, t_total, hirm);
    }
    // TRANSITION IRMs.
    for (const auto& [t, irm] : hirm->irms) {
      single_step_irm_inference(prng, irm, t_total, verbose);
    }
  }
}

int main(int argc, char** argv) {
  cxxopts::Options options("hirm",
                           "Run a hierarchical infinite relational model.");
  options.add_options()("help", "show help message")(
      "mode", "options are {irm, hirm}",
      cxxopts::value<std::string>()->default_value("hirm"))(
      "seed", "random seed", cxxopts::value<int>()->default_value("10"))(
      "iters", "number of inference iterations",
      cxxopts::value<int>()->default_value("10"))(
      "verbose", "report results to terminal",
      cxxopts::value<bool>()->default_value("false"))(
      "timeout", "number of seconds of inference",
      cxxopts::value<int>()->default_value("0"))(
      "load", "path to .[h]irm file with initial clusters",
      cxxopts::value<std::string>()->default_value(""))(
      "path", "base name of the .schema file", cxxopts::value<std::string>())(
      "rest", "rest",
      cxxopts::value<std::vector<std::string>>()->default_value({}));
  options.parse_positional({"path", "rest"});
  options.positional_help("<path>");

  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if (result.count("path") == 0) {
    std::cout << options.help() << std::endl;
    return 1;
  }

  std::string path_base = result["path"].as<std::string>();
  int seed = result["seed"].as<int>();
  int iters = result["iters"].as<int>();
  int timeout = result["timeout"].as<int>();
  bool verbose = result["verbose"].as<bool>();
  std::string path_clusters = result["load"].as<std::string>();
  std::string mode = result["mode"].as<std::string>();

  if (mode != "hirm" && mode != "irm") {
    std::cout << options.help() << std::endl;
    std::cout << "unknown mode " << mode << std::endl;
    return 1;
  }

  std::string path_obs = path_base + ".obs";
  std::string path_schema = path_base + ".schema";
  std::string path_save = path_base + "." + std::to_string(seed);

  printf("setting seed to %d\n", seed);
  std::mt19937 prng(seed);

  std::cout << "loading schema from " << path_schema << std::endl;
  auto schema = load_schema(path_schema);

  std::cout << "loading observations from " << path_obs << std::endl;
  auto observations = load_observations(path_obs, schema);
  auto encoding = encode_observations(schema, observations);

  if (mode == "irm") {
    std::cout << "selected model is IRM" << std::endl;
    IRM* irm;
    // Load
    if (path_clusters.empty()) {
      irm = new IRM(schema);
      std::cout << "incorporating observations" << std::endl;
      incorporate_observations(&prng, *irm, encoding, observations);
    } else {
      irm = new IRM({});
      std::cout << "loading clusters from " << path_clusters << std::endl;
      from_txt(&prng, irm, path_schema, path_obs, path_clusters);
    }
    // Infer
    std::cout << "inferring " << iters << " iters; timeout " << timeout
              << std::endl;
    inference_irm(&prng, irm, iters, timeout, verbose);
    // Save
    path_save += ".irm";
    std::cout << "saving to " << path_save << std::endl;
    to_txt(path_save, *irm, encoding);
    // Free
    free(irm);
    return 0;
  }

  if (mode == "hirm") {
    std::cout << "selected model is HIRM" << std::endl;
    HIRM* hirm;
    // Load
    if (path_clusters.empty()) {
      hirm = new HIRM(schema, &prng);
      std::cout << "incorporating observations" << std::endl;
      incorporate_observations(&prng, *hirm, encoding, observations);
    } else {
      hirm = new HIRM({}, &prng);
      std::cout << "loading clusters from " << path_clusters << std::endl;
      from_txt(&prng, hirm, path_schema, path_obs, path_clusters);
    }
    // Infer
    std::cout << "inferring " << iters << " iters; timeout " << timeout
              << std::endl;
    inference_hirm(&prng, hirm, iters, timeout, verbose);
    // Save
    path_save += ".hirm";
    std::cout << "saving to " << path_save << std::endl;
    to_txt(path_save, *hirm, encoding);
    // Free
    free(hirm);
    return 0;
  }
}
