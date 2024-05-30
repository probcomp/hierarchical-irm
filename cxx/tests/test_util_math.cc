// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cassert>
#include <cstdio>
#include <vector>

#include "util_math.hh"

int main(int argc, char **argv) {
  std::vector<std::vector<int>> x{{1}, {2, 3}, {1, 10, 11}};

  auto cartesian = product(x);
  assert(cartesian.size() == 6);
  assert((cartesian.at(0) == std::vector<int>{1, 2, 1}));
  assert((cartesian.at(1) == std::vector<int>{1, 2, 10}));
  assert((cartesian.at(2) == std::vector<int>{1, 2, 11}));
  assert((cartesian.at(3) == std::vector<int>{1, 3, 1}));
  assert((cartesian.at(4) == std::vector<int>{1, 3, 10}));
  assert((cartesian.at(5) == std::vector<int>{1, 3, 11}));

  x.push_back({});
  cartesian = product(x);
  assert(cartesian.size() == 0);
}
