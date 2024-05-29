// Copyright 2024
// See LICENSE.txt

// A class for turning Distribution<SampleType>'s into
// Distribution<string>'s.

#include "distributions/adapter.hh"
#include "distributions/base.hh"
#include "distributions/beta_bernoulli.hh"
#include "distributions/bigram.hh"
#include "distributions/dirichlet_categorical.hh"
#include "distributions/normal.hh"

// have return type be variant?
// https://stackoverflow.com/questions/44870307/a-function-overload-depending-on-enum
Distribution<std::string>* get_distribution(const std::string& dist_str) {
  // do enum?
  std::mt19937 prng;
  BetaBernoulli bb (&prng);
  return new DistributionAdapter<double>(&bb);
//   if (dist_str == "normal") {
//     Normal d (&prng);
//     auto da = new DistributionAdapter<double>(&d);
//     return da;
//   } else if (dist_str == "bigram") {
//     Bigram bg (&prng);
//     return new DistributionAdapter<std::string>(&bg);
//   } else if (dist_str == "beta_bernoulli") {
//     BetaBernoulli bb (&prng);
//     return new DistributionAdapter<double>(&bb);
//   // } else if (dist_str == "dirichlet_categorical") {
//   } else {
//     DirichletCategorical dc (&prng, 5);  // how to pass in number of categories?
//     return new DistributionAdapter<double>(&dc);
//   };
}