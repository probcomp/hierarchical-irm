// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cstdio>
#include <ctime>
#include <iostream>

#include "cxxopts.hpp"
#include "hirm.hh"
#include "inference.hh"
#include "irm.hh"
#include "util_io.hh"

int main(int argc, char** argv) {
  cxxopts::Options options("hirm",
                           "Run a hierarchical infinite relational model.");
  options.add_options()("help", "show help message")(
      "mode", "options are {irm, hirm}",
      cxxopts::value<std::string>()->default_value("hirm"))(
      "seed", "random seed", cxxopts::value<int>()->default_value("10"))(
      "iters", "number of inference iterations",
      cxxopts::value<int>()->default_value("10"))(
      "verbose", "report results to terminal",
      cxxopts::value<bool>()->default_value("false"))(
      "timeout", "number of seconds of inference",
      cxxopts::value<int>()->default_value("0"))(
      "samples", "number of samples to write to disk",
      cxxopts::value<int>()->default_value("0"))(
      "load", "path to .[h]irm file with initial clusters",
      cxxopts::value<std::string>()->default_value(""))(
      "path", "base name of the .schema file", cxxopts::value<std::string>())(
      "heldout", "filename of held-out observations",
      cxxopts::value<std::string>()->default_value(""))(
      "rest", "rest",
      cxxopts::value<std::vector<std::string>>()->default_value({}));
  options.parse_positional({"path", "rest"});
  options.positional_help("<path>");

  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if (result.count("path") == 0) {
    std::cout << options.help() << std::endl;
    return 1;
  }

  std::string path_base = result["path"].as<std::string>();
  int seed = result["seed"].as<int>();
  int iters = result["iters"].as<int>();
  int timeout = result["timeout"].as<int>();
  bool verbose = result["verbose"].as<bool>();
  int num_samples = result["samples"].as<int>();
  std::string path_clusters = result["load"].as<std::string>();
  std::string mode = result["mode"].as<std::string>();

  if (mode != "hirm" && mode != "irm") {
    std::cout << options.help() << std::endl;
    std::cout << "unknown mode " << mode << std::endl;
    return 1;
  }

  std::string path_obs = path_base + ".obs";
  std::string path_schema = path_base + ".schema";
  std::string path_save = path_base + "." + std::to_string(seed);

  printf("setting seed to %d\n", seed);
  std::mt19937 prng(seed);

  std::cout << "loading schema from " << path_schema << std::endl;
  T_schema schema = load_schema(path_schema);

  std::cout << "loading observations from " << path_obs << std::endl;
  T_observations observations = load_observations(path_obs, schema);

  std::string held_out = result["heldout"].as<std::string>();
  T_observations heldout_obs;
  T_observations encoding_observations;
  if (held_out.empty()) {
    encoding_observations = observations;
  } else {
    std::cout << "loading held out observations from " << held_out << std::endl;
    heldout_obs = load_observations(held_out, schema);
    encoding_observations = merge_observations(observations, heldout_obs);
  }

  std::cout << "Encoding observations ...\n";
  T_encoding encoding = calculate_encoding(schema, encoding_observations);

  if (mode == "irm") {
    std::cout << "selected model is IRM" << std::endl;
    IRM* irm;
    // Load
    if (path_clusters.empty()) {
      irm = new IRM(schema);
      std::cout << "incorporating observations" << std::endl;
      incorporate_observations(&prng, irm, encoding, observations);
    } else {
      irm = new IRM({});
      std::cout << "loading clusters from " << path_clusters << std::endl;
      from_txt(&prng, irm, path_schema, path_obs, path_clusters);
    }
    // Infer
    std::cout << "inferring " << iters << " iters; timeout " << timeout
              << std::endl;
    inference_irm(&prng, irm, iters, timeout, verbose);
    // Save
    path_save += ".irm";
    std::cout << "saving to " << path_save << std::endl;
    to_txt(path_save, *irm, encoding);

    if (!held_out.empty()) {
      double lp = logp(&prng, irm, encoding, heldout_obs);
      std::cout << "Log likelihood of held out data is " << lp << std::endl;
    }

    if (num_samples > 0) {
      std::cout << "Sorry, IRM does not currently support generating samples\n";
    }

    // Free
    free(irm);
    return 0;
  }

  if (mode == "hirm") {
    std::cout << "selected model is HIRM" << std::endl;
    HIRM* hirm;
    // Load
    if (path_clusters.empty()) {
      hirm = new HIRM(schema, &prng);
      std::cout << "incorporating observations" << std::endl;
      incorporate_observations(&prng, hirm, encoding, observations);
    } else {
      hirm = new HIRM({}, &prng);
      std::cout << "loading clusters from " << path_clusters << std::endl;
      from_txt(&prng, hirm, path_schema, path_obs, path_clusters);
    }
    // Infer
    std::cout << "inferring " << iters << " iters; timeout " << timeout
              << std::endl;
    inference_hirm(&prng, hirm, iters, timeout, verbose);
    // Save
    path_save += ".hirm";
    std::cout << "saving to " << path_save << std::endl;
    to_txt(path_save, *hirm, encoding);

    if (!held_out.empty()) {
      double lp = logp(&prng, hirm, encoding, heldout_obs);
      std::cout << "Log likelihood of held out data is " << lp << std::endl;
    }

    if (num_samples > 0) {
      std::cout << "Generating " << num_samples << " samples\n";
      T_encoded_observations sampled_obs = hirm->sample_and_incorporate(
          &prng, num_samples);
      std::cout << "Writing samples ...\n";
      write_observations(path_save + ".samples", sampled_obs, encoding, hirm);
    }

    // Free
    free(hirm);
    return 0;
  }
}
