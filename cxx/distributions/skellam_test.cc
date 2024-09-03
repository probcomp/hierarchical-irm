// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Skellam

#include "distributions/skellam.hh"

#include <boost/test/included/unit_test.hpp>

namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(simple) {
  Skellam sd;
  std::mt19937 prng;

  sd.init_theta(&prng);

  BOOST_TEST(sd.logp_score() == 0.0, tt::tolerance(1e-6));
  BOOST_TEST(sd.logp(6) == -8.2461659399497425, tt::tolerance(1e-6));

  sd.incorporate(5);
  sd.incorporate(2);
  BOOST_TEST(sd.N == 2);

  sd.unincorporate(5);
  sd.incorporate(7);
  BOOST_TEST(sd.N == 2);

  BOOST_TEST(sd.logp_score() == -12.676907210873877, tt::tolerance(1e-6));
  BOOST_TEST(sd.logp(6) == -8.2461659399497425, tt::tolerance(1e-6));

  int s = sd.sample(&prng);
  BOOST_TEST(s < 100.0);
}

BOOST_AUTO_TEST_CASE(set_and_store_latents) {
  Skellam sd;
  std::mt19937 prng;

  sd.init_theta(&prng);
  std::vector<double> v = sd.store_latents();

  sd.init_theta(&prng);
  BOOST_TEST(v != sd.store_latents(), tt::per_element());

  sd.set_latents(v);

  BOOST_TEST(v == sd.store_latents(), tt::per_element());
}

BOOST_AUTO_TEST_CASE(transition_theta) {
  Skellam sd;
  std::mt19937 prng;

  sd.init_theta(&prng);

  for (int i = 0; i < 100; ++i) {
    sd.incorporate(5);
    sd.incorporate(6);
    sd.incorporate(6);
    sd.incorporate(7);
  }

  for (int i = 0; i < 100; ++i) {
    sd.transition_theta(&prng);
  }

  BOOST_TEST(sd.mu1 > sd.mu2);
}

// Enable when Bessel function is better behaved.
// BOOST_AUTO_TEST_CASE(test_sample_and_log_prob) {
//   std::mt19937 prng;
//   Skellam sd;
//   for (int i = 0; i < 100; ++i) {
//     sd.incorporate(5);
//     sd.incorporate(6);
//     sd.incorporate(6);
//     sd.incorporate(7);
//   }
// 
//   for (int i = 0; i < 100; ++i) {
//     sd.transition_theta(&prng);
//   }
// 
//   std::map<int, int> counts;
//   const int num_samples = 100000;
//   for (int i = 0; i < num_samples; ++i) {
//     double sample = sd.sample(&prng);
//     if (counts.contains(sample)) {
//       ++counts[sd.sample(&prng)];
//     }
//     else {
//       counts[sd.sample(&prng)] = 1;
//     }
//   }
// 
//   std::map<int, double> probs;
//   for (const auto& kv : counts) {
//     probs[kv.first] = exp(sd.logp(kv.first));
//   }
//   double stddev = sqrt(sd.mu1 + sd.mu2);
// 
//   // Check that we are within 3 standard deviations
//   for (const auto& kv : counts) {
//     double approx_p = kv.second / num_samples;
//     BOOST_TEST(abs(probs[kv.first] - approx_p) <= 3 * stddev);
//   }
// }
