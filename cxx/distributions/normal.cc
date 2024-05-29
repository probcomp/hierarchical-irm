// Copyright 2024
// See LICENSE.txt

#include <cmath>
#include <numbers>
#include "normal.hh"

double logZ(double r, double v, double s) {
  return (v + 1.0) / 2.0 * log(2.0)
      + 0.5 * log(std::numbers::pi)
      - 0.5 * log(r)
      - 0.5 * v * log(s)
      + lgamma(0.5 * v);
}
