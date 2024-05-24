// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include <algorithm>
#include <random>

#include "util_math.hh"

// http://matlab.izmiran.ru/help/techdoc/ref/betaln.html
double lbeta(int z, int w) {
    return lgamma(z) + lgamma(w) - lgamma(z + w);
}

std::vector<double> linspace(double start, double stop, int num, bool endpoint) {
    double step = (stop - start) / (num - endpoint);
    std::vector<double> v;
    for (int i = 0; i < num; i++) {
        v.push_back(start + step * i);
    }
    return v;
}

std::vector<double> log_linspace(double start, double stop, int num, bool endpoint) {
    auto v = linspace(log(start), log(stop), num, endpoint);
    for (int i = 0; i < v.size(); i++) {
        v[i] = exp(v[i]);
    }
    return v;
}

std::vector<double> log_normalize(const std::vector<double> &weights){
    double Z = logsumexp(weights);
    std::vector<double> result(weights.size());
    for (int i = 0; i < weights.size(); i++) {
        result[i] = weights[i] - Z;
    }
    return result;
}

double logsumexp(const std::vector<double> &weights) {
    // Get the max index.
    int max_index = std::distance(
        weights.cbegin(), std::max_element(weights.cbegin(), weights.cend()));
    double m = weights[max_index];
    double s = 0;
    for (int i = 0; i < weights.size(); ++i) {
      if (i == max_index) {
        continue;
      }
      if (std::isfinite(weights[i])) {
        s += exp(weights[i] - m);
      }
    }
    return log1p(s) + m;
}

int choice(const std::vector<double> &weights, std::mt19937 *prng) {
    std::discrete_distribution<int> dist(weights.begin(), weights.end());
    int idx = dist(*prng);
    return idx;
}

int log_choice(const std::vector<double> &weights, std::mt19937 *prng) {
    std::vector<double> log_weights_norm = log_normalize(weights);
    std::vector<double> weights_norm;
    for (double w : log_weights_norm) {
        weights_norm.push_back(exp(w));
    }
    return choice(weights_norm, prng);
}

std::vector<std::vector<int>> product(const std::vector<std::vector<int>> &lists) {
    // https://rosettacode.org/wiki/Cartesian_product_of_two_or_more_lists#C.2B.2B
    std::vector<std::vector<int>> result;
    for (const auto &l : lists) {
        if (l.empty()) {
            return result;
        }
    }
    for (const int &e : lists[0]) {
        result.push_back({e});
    }
    for (size_t i = 1; i < lists.size(); ++i) {
        std::vector<std::vector<int>> temp;
        for (std::vector<int> &e : result) {
            for (int f : lists[i]) {
                std::vector<int> e_tmp = e;
                e_tmp.push_back(f);
                temp.push_back(e_tmp);
          }
        }
        result = temp;
      }
      return result;
    }
