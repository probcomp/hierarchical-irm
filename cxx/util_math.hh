// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <random>
#include <vector>

double lbeta(int z, int w);

std::vector<double> linspace(double start, double stop, int num, bool endpoint);
std::vector<double> log_linspace(double start, double stop, int num,
                                 bool endpoint);
std::vector<double> log_normalize(const std::vector<double>& weights);
double logsumexp(const std::vector<double>& weights);

int choice(const std::vector<double>& weights, std::mt19937* prng);
int log_choice(const std::vector<double>& weights, std::mt19937* prng);

std::vector<std::vector<int>> product(
    const std::vector<std::vector<int>>& lists);
