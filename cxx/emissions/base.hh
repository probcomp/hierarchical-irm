#pragma once

#include <cstdio>
#include <cstdlib>
#include <utility>
#include "distributions/base.hh"

template <typename SampleType = double>
class Emission : public Distribution<std::pair<SampleType, SampleType>> {
  virtual std::pair<SampleType, SampleType> sample() {
    printf("sample() should never be called on an Emission\n");
    std::abort();
  }

  // Return a stochastically corrupted version of clean.
  virtual SampleType sample_corrupted(SampleType clean, std::mt19937 *prng) = 0;
};
