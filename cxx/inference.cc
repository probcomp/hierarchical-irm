// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cstdio>
#include <ctime>
#include "inference.hh"

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

void inference_irm(std::mt19937* prng, IRM* irm, int iters, int timeout,
                   bool verbose) {
  clock_t t_begin = clock();
  double t_total = 0;
  for (int i = 0; i < iters; ++i) {
    printf("Starting iteration %d, model score = %f\n",
           i+1, irm->logp_score());
    CHECK_TIMEOUT(timeout, t_begin);
    single_step_irm_inference(prng, irm, t_total, verbose, 10, true);
  }
}

void inference_hirm(std::mt19937* prng, HIRM* hirm, int iters, int timeout,
                    bool verbose) {
  clock_t t_begin = clock();
  double t_total = 0;
  for (int i = 0; i < iters; ++i) {
    printf("Starting iteration %d, model score = %f\n",
           i+1, hirm->logp_score());
    CHECK_TIMEOUT(timeout, t_begin);
    // TRANSITION LATENT VALUES.
    for (const auto& [rel, nrels] : hirm->base_to_noisy_relations) {
      if (std::visit([](const auto& s) { return !s.is_observed; },
                     hirm->schema.at(rel))) {
        clock_t t = clock();
        hirm->transition_latent_values_relation(prng, rel);
        REPORT_SCORE(verbose, t, t_total, hirm);
      }
    }
    // TRANSITION RELATIONS.
    for (const auto& [r, rc] : hirm->relation_to_code) {
      clock_t t = clock();
      hirm->transition_cluster_assignment_relation(prng, r);
      REPORT_SCORE(verbose, t, t_total, hirm);
    }
    // TRANSITION IRMs.
    for (const auto& [t, irm] : hirm->irms) {
      single_step_irm_inference(prng, irm, t_total, verbose, 10, false);
    }
  }
}


