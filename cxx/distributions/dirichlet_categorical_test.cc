// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test DirichletCategorical

#include "distributions/dirichlet_categorical.hh"

#include <boost/test/included/unit_test.hpp>

#include "distributions/beta_bernoulli.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_matches_beta_bernoulli) {
  // 2-category Dirichlet Categorical is the same as a BetaBernoulli.
  DirichletCategorical dc(2);
  BetaBernoulli bb;

  for (int i = 0; i < 10; ++i) {
    dc.incorporate(i % 2);
    bb.incorporate(i % 2);
  }
  for (int i = 0; i < 10; i += 2) {
    dc.unincorporate(i % 2);
    bb.unincorporate(i % 2);
  }

  BOOST_TEST(dc.N == 5);

  BOOST_TEST(dc.logp(1) == bb.logp(1), tt::tolerance(1e-6));
  BOOST_TEST(dc.logp(0) == bb.logp(0), tt::tolerance(1e-6));
  BOOST_TEST(dc.logp_score() == bb.logp_score(), tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_logp_score) {
  // Sample from a Dirichlet and use that to create a MC estimate of the log
  // prob.
  std::mt19937 prng;

  DirichletCategorical dc(5);

  int num_samples = 1000;
  std::vector<std::vector<double>> dirichlet_samples;
  std::gamma_distribution<> gamma_dist(dc.alpha, 1.);
  for (int i = 0; i < num_samples; ++i) {
    std::vector<double> sample;
    for (int j = 0; j < 5; ++j) {
      sample.emplace_back(gamma_dist(prng));
    }
    double sum_of_elements = std::accumulate(sample.begin(), sample.end(), 0.);
    for (int j = 0; j < 5; ++j) {
      sample[j] /= sum_of_elements;
    }
    dirichlet_samples.emplace_back(sample);
  }

  for (int i = 0; i < 20; ++i) {
    dc.incorporate(i % 5);
    double average_prob = 0;
    for (const auto& sample : dirichlet_samples) {
      double prob = 1;
      for (int j = 0; j <= i; ++j) {
        prob *= sample[j % 5];
      }
      average_prob += prob;
    }
    average_prob /= num_samples;
    BOOST_TEST(dc.logp_score() == log(average_prob), tt::tolerance(8e-3));
  }
}

BOOST_AUTO_TEST_CASE(test_logp) {
  // Sample from a Dirichlet and use that to create a MC estimate of the log
  // prob.
  std::mt19937 prng;
  int num_categories = 5;

  DirichletCategorical dc(num_categories);

  // We'll use the fact that the posterior distribution of a
  // DirichletCategorical is a Dirichlet.
  // Thus we only need to compute an expectation with respect to
  // this Dirichlet posterior for the posterior predictive.
  std::vector<double> effective_concentration;
  for (int i = 0; i < num_categories; ++i) {
    effective_concentration.emplace_back(dc.alpha);
  }
  int num_samples = 10000;

  for (int i = 0; i < 20; ++i) {
    dc.incorporate(i % num_categories);
    ++effective_concentration[i % num_categories];

    int test_data = (i * i) % num_categories;
    double average_prob = 0;

    std::vector<std::gamma_distribution<>> gamma_dists;
    for (int j = 0; j < num_categories; ++j) {
      gamma_dists.emplace_back(
          std::gamma_distribution<>(effective_concentration[j], 1.));
    }

    // Create samples with these concentration parameters.
    for (int j = 0; j < num_samples; ++j) {
      std::vector<double> sample;
      for (int k = 0; k < num_categories; ++k) {
        sample.emplace_back(gamma_dists[k](prng));
      }
      double sum_of_elements =
          std::accumulate(sample.begin(), sample.end(), 0.);
      for (int k = 0; k < num_categories; ++k) {
        sample[k] /= sum_of_elements;
      }
      // For this posterior sample, compute an estimate of the posterior
      // predictive on the test data point.
      average_prob += sample[test_data];
    }
    average_prob /= num_samples;
    BOOST_TEST(dc.logp(test_data) == log(average_prob), tt::tolerance(8e-3));
  }
}

BOOST_AUTO_TEST_CASE(test_transition_hyperparameters) {
  std::mt19937 prng;
  DirichletCategorical dc(10);

  dc.transition_hyperparameters(&prng);

  for (int i = 0; i < 100; ++i) {
    dc.incorporate(i % 10);
  }

  BOOST_TEST(dc.N == 100);
  dc.transition_hyperparameters(&prng);
  BOOST_TEST(dc.alpha > 1.0);
}

BOOST_AUTO_TEST_CASE(test_sample_and_log_prob) {
  std::mt19937 prng;
  DirichletCategorical dc(5);
  std::vector<double> primes {2, 3, 5, 7, 11};

  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < primes[i]; ++j) {
      dc.incorporate(i);
    }
  }

  std::vector<int> counts{0, 0, 0, 0, 0};
  const int num_samples = 100000;
  for (int i = 0; i < num_samples; ++i) {
    ++counts[dc.sample(&prng)];
  }

  std::vector<double> probs;
  for (int i = 0; i < 5; ++i) {
    probs.push_back(exp(dc.logp(i)));
  }

  // Check that we are within 3 standard deviations
  for (int i = 0; i < 5; ++i) {
    double approx_p = 1. * counts[i] / num_samples;
    double stddev = sqrt(probs[i] * (1 - probs[i]) / num_samples);
    BOOST_TEST(abs(probs[i] - approx_p) <= 3 * stddev);
  }
}
