// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

// Example usage:
//   ./pclean --schema=xxx.schema --obs=xxx.obs

#include <iostream>
#include <random>

#include "cxxopts.hpp"
#include "irm.hh"
#include "hirm.hh"
#include "inference.hh"
#include "util_io.hh"
#include "pclean/csv.hh"
#include "pclean/io.hh"
#include "pclean/schema.hh"
#include "pclean/schema_helper.hh"

T_observations translate_observations(
    const DataFrame& df, const T_schema &schema) {
  T_observations obs;

  for (const auto& col : df.data) {
    const std::string& col_name = col.first;
    const T_relation& trel = schema.at(col_name);
    size_t num_domains;
    std::visit([&](const auto &r) {
      num_domains = r.domains.size();
    }, trel);

    for (size_t i = 0; i < col.second.size(); ++i) {
      const std::string& val = col.second[i];
      if (val.empty()) {
        // Don't incorporate missing values.
        // TODO(thomaswc): Allow the user to specify other values that mean
        // missing data.  ("missing", "NA", "nan", etc.).
        continue;
      }

      std::vector<std::string> entities;
      for (size_t j = 0; j < num_domains; ++j) {
        // Assume that each row of the dataframe is its own entity, *and*
        // that all of its ancestor entities are distinct from those of any
        // other entity.
        entities.push_back(std::to_string(i));
      }
      obs[col_name].push_back(std::make_tuple(entities, val));
    }
  }
  return obs;
}

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
  std::cout << "Reading schema file from " << schema_fn << "\n";
  if (!read_schema_file(schema_fn, &pclean_schema)) {
    std::cout << "Error reading schema file" << schema_fn << "\n";
  }

  // Translate schema
  std::cout << "Translating schema ...\n";
  PCleanSchemaHelper schema_helper(pclean_schema);
  T_schema hirm_schema = schema_helper.make_hirm_schema();

  // Read observations
  std::string obs_fn = result["obs"].as<std::string>();
  std::cout << "Reading observations file from " << obs_fn << "\n";
  DataFrame df = DataFrame::from_csv(obs_fn);

  // Validate that we have a relation for each observation column.
  for (const auto &col : df.data) {
    if (!hirm_schema.contains(col.first)) {
      printf("Error, could not find HIRM relation for column %s\n",
             col.first.c_str());
      assert(false);
    }
  }

  // Create model
  HIRM hirm(hirm_schema, &prng);

  // Incorporate observations.
  std::cout << "Incorporating observations ...\n";
  T_observations observations = translate_observations(df, hirm_schema);
  T_encoding encoding = encode_observations(hirm_schema, observations);
  incorporate_observations(&prng, &hirm, encoding, observations);

  // Run inference
  std::cout << "Running inference ...\n";
  inference_hirm(&prng, &hirm,
                 result["iters"].as<int>(),
                 result["timeout"].as<int>(),
                 result["verbose"].as<bool>());

  // Save results
  // TODO(thomaswc): This

  return 0;
}
