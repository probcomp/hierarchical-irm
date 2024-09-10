// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

// Example usage:
//   ./pclean --schema=assets/flights.schema --obs=assets/flights_dirty.csv
//            --iters=5

#include <iostream>
#include <random>

#include "cxxopts.hpp"
#include "irm.hh"
#include "hirm.hh"
#include "inference.hh"
#include "util_io.hh"
#include "pclean/csv.hh"
#include "pclean/io.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema.hh"
#include "pclean/schema_helper.hh"

int main(int argc, char** argv) {
  cxxopts::Options options(
      "pclean", "Run HIRM from a PClean schema");
  options.add_options()
      ("h,help", "Show help message")
      ("s,schema", "Filename of .schema input", cxxopts::value<std::string>())
      ("obs", "Filename of observations input", cxxopts::value<std::string>())
      ("output", "Filename of relation clusters output",
       cxxopts::value<std::string>())
      ("heldout", "Filename of heldout observations",
       cxxopts::value<std::string>())
      ("i,iters", "Number of inference iterations",
       cxxopts::value<int>()->default_value("10"))
      ("seed", "Random seed", cxxopts::value<int>()->default_value("10"))
      ("samples", "Number of samples to generate",
       cxxopts::value<int>()->default_value("0"))
      ("only_final_emissions", "Only create one layer of emissions",
       cxxopts::value<bool>()->default_value("false"))
      ("record_class_is_clean",
       "If false, model queries of the query class with emissions noise.",
       cxxopts::value<bool>()->default_value("true"))
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
  std::cout << "Reading plcean schema ...\n";
  PCleanSchema pclean_schema;
  std::string schema_fn = result["schema"].as<std::string>();
  std::cout << "Reading schema file from " << schema_fn << "\n";
  if (!read_schema_file(schema_fn, &pclean_schema)) {
    std::cout << "Error reading schema file" << schema_fn << "\n";
  }

  // Translate schema
  std::cout << "Making schema helper ...\n";
  PCleanSchemaHelper schema_helper(
      pclean_schema,
      result["only_final_emissions"].as<bool>(),
      result["record_class_is_clean"].as<bool>());
  std::cout << "Translating schema ...\n";
  std::map<std::string, std::vector<std::string>> annotated_domains_for_relations;
  T_schema hirm_schema = schema_helper.make_hirm_schema(
      &annotated_domains_for_relations);

  // Read observations
  std::cout << "Reading observations ...\n";
  std::string obs_fn = result["obs"].as<std::string>();
  std::cout << "Reading observations file from " << obs_fn << "\n";
  DataFrame df = DataFrame::from_csv(obs_fn);

  // Create model
  std::cout << "Creating hirm ...\n";
  HIRM hirm(hirm_schema, &prng);

  // Incorporate observations.
  std::cout << "Translating observations ...\n";
  T_observations observations = translate_observations(
      df, hirm_schema, annotated_domains_for_relations);

  std::string heldout_fn = result["heldout"].as<std::string>();
  T_observations heldout_obs;
  T_observations encoding_observations;
  if (heldout_fn.empty()) {
    encoding_observations = observations;
  } else {
    std::cout << "Loading held out observations from " << heldout_fn << std::endl;
    DataFrame heldout_df = DataFrame::from_csv(heldout_fn);
    heldout_obs = translate_observations(
        heldout_df, hirm_schema, annotated_domains_for_relations);
    encoding_observations = merge_observations(observations, heldout_obs);
  }

  std::cout << "Encoding observations ...\n";
  T_encoding encoding = calculate_encoding(hirm_schema, encoding_observations);

  std::cout << "Incorporating observations ...\n";
  incorporate_observations(&prng, &hirm, encoding, observations);

  // Run inference
  std::cout << "Running inference ...\n";
  inference_hirm(&prng, &hirm,
                 result["iters"].as<int>(),
                 result["timeout"].as<int>(),
                 result["verbose"].as<bool>());

  // Save results
  if (result.count("output") > 0) {
    std::string out_fn = result["output"].as<std::string>();
    std::cout << "Savings results to " << out_fn << "\n";
    to_txt(out_fn, hirm, encoding);
  }

  if (!heldout_fn.empty()) {
    double lp = logp(&prng, &hirm, encoding, heldout_obs);
    std::cout << "Log likelihood of held out data is " << lp << std::endl;
  }

  int num_samples = result["samples"].as<int>();
  if (num_samples > 0) {
    std::string samples_out = result["output"].as<std::string>() + ".samples";
    std::cout << "Generating " << num_samples << " samples\n";
    DataFrame samples_df = make_pclean_samples(num_samples, hirm, &prng);
    std::cout << "Writing samples to " << samples_out << " ...\n";
    samples_df.to_csv(samples_out);
  }

  return 0;
}
