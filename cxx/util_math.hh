// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <random>
#include <vector>

double lbeta(double z, double w);

std::vector<double> linspace(double start, double stop, int num, bool endpoint);
std::vector<double> log_linspace(double start, double stop, int num,
                                 bool endpoint);
std::vector<double> log_normalize(const std::vector<double> &weights);
double logsumexp(const std::vector<double> &weights);

int choice(const std::vector<double> &weights, std::mt19937 *prng);
int log_choice(const std::vector<double> &weights, std::mt19937 *prng);

std::vector<std::vector<int>> product(
    const std::vector<std::vector<int>> &lists);

// Given a vector of log probabilities, return a sample.
int sample_from_logps(const std::vector<double> &log_probs, std::mt19937 *prng);
