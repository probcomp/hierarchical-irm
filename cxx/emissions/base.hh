#pragma once

#include <cstdio>
#include <cstdlib>
#include <random>
#include <utility>

#include "distributions/base.hh"

template <typename SampleType = double>
class Emission : public Distribution<std::pair<SampleType, SampleType>> {
 public:
  virtual std::pair<SampleType, SampleType> sample() {
    printf("sample() should never be called on an Emission\n");
    std::abort();
  }

  // Return a stochastically corrupted version of clean.
  virtual SampleType sample_corrupted(const SampleType& clean,
                                      std::mt19937* prng) = 0;

  // Propose a clean value given a vector of corrupted values.
  virtual SampleType propose_clean(const std::vector<SampleType>& corrupted,
                                   std::mt19937* prng) = 0;
};
