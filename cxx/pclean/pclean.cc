// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

// Example usage:
//   ./pclean --schema=xxx.schema --obs=xxx.obs

#include <iostream>
#include <random>

#include "cxxopts.hpp"
#include "hirm.hh"
#include "inference.hh"
#include "pclean/io.hh"
#include "pclean/schema.hh"

int main(int argc, char** argv) {
  cxxopts::Options options(
      "pclean", "Run HIRM from a PClean schema");
  options.add_options()
      ("h,help", "Show help message")
      ("s,schema", "Filename of .schema input", cxxopts::value<std::string>())
      ("o,obs", "Filename of observations input", cxxopts::value<std::string>())
      ("i,iters", "Number of inference iterations",
       cxxopts::value<int>()->default_value("10"))
      ("seed", "Random seed", cxxopts::value<int>()->default_value("10"))
      ("t,timeout", "Timeout in seconds for inference",
       cxxopts::value<int>()->default_value("0"))
      ("v,verbose", "Verbose output",
       cxxopts::value<bool>()->default_value("false"))
  ;
  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if ((result.count("schema") < 1) || (result.count("obs") < 1)) {
    std::cout << "Must specify --schema and --obs\n";
    return 0;
  }

  int seed = result["seed"].as<int>();
  std::cout << "Setting seed to " << seed << "\n";
  std::mt19937 prng(seed);

  // Read schema
  PCleanSchema pclean_schema;
  std::string schema_fn = result["schema"].as<std::string>();
  if (!read_schema_file(schema_fn, &pclean_schema)) {
    std::cout << "Error reading schema file" << schema_fn << "\n";
  }

  // Translate schema
  PCleanSchemaHelper schema_helper(pclean_schema);
  T_schema hirm_schema = schema_helper.make_hirm_schema();

  // Read observations
  std::string obs_fn = result["obs"].as<std::string>();
  // TODO(thomaswc): This

  // Create model
  HIRM hirm(hirm_schema, &prng);

  // Incorporate observations.
  // TODO(thomaswc): This

  // Run inference
  inference_hirm(&prng, &hirm,
                 result["iters"].as<int>(),
                 result["timeout"].as<int>(),
                 result["verbose"].as<bool>());

  // Save results
  // TODO(thomaswc): This

  return 0;
}