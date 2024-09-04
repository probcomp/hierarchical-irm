// Copyright 2020
// See LICENSE.txt

#pragma once

#include "hirm.hh"
#include "observations.hh"

typedef std::map<std::string, std::map<std::string, T_item>> T_encoding_f;
typedef std::map<std::string, std::map<T_item, std::string>> T_encoding_r;
typedef std::tuple<T_encoding_f, T_encoding_r> T_encoding;

// Load the schema file from path.  Exits if the schema file can't be parsed.
T_schema load_schema(const std::string& path);

// Load the observations from the CSV file at path.  Also marks all observed
// relations in schema as is_observed = true.
T_observations load_observations(const std::string& path, T_schema& schema);

// Write the observations to path.
void write_observations(
    const std::string& path, const T_encoded_observations& observations,
    const& T_encoding encoding);

// Calculates an encoding for the observations: in the input T_observations,
// domain values are denoted by strings, and the encoding maps those to/from
// integers.
T_encoding calculate_encoding(
    const T_schema& schema, const T_observations& observations);

// Returns the encoded version of the input observations.
T_encoded_observations encode_observations(
    const T_observations& observations, const T_encoding& encoding,
    std::variant<IRM*, HIRM*> h_irm);

void incorporate_observations_relation(
    std::mt19937* prng, const std::string& relation,
    std::variant<IRM*, HIRM*> h_irm, const T_encoded_observations& observations,
    std::unordered_map<std::string, std::string>& noisy_to_base,
    std::unordered_map<std::string, std::unordered_set<T_items, H_items>>&
        relation_items,
    std::unordered_set<std::string>& completed_relations);
void incorporate_observations(std::mt19937* prng, std::variant<IRM*, HIRM*> h_irm,
                              const T_encoding& encoding,
                              const T_observations& observations);
void to_txt(const std::string& path, const IRM& irm,
            const T_encoding& encoding);
void to_txt(const std::string& path, const HIRM& irm,
            const T_encoding& encoding);
void to_txt(std::ostream& fp, const IRM& irm, const T_encoding& encoding);
void to_txt(std::ostream& fp, const HIRM& irm, const T_encoding& encoding);

std::map<std::string, std::map<int, std::vector<std::string>>>
load_clusters_irm(const std::string& path);
std::tuple<std::map<int, std::vector<std::string>>,  // x[table] = {relation
                                                     // list}
           std::map<
               int,
               std::map<
                   std::string,
                   std::map<int, std::vector<
                                     std::string>>>>  // x[table][domain][table]
                                                      // =
                                                      // {item
                                                      // list}
           >
load_clusters_hirm(const std::string& path);

void from_txt(std::mt19937* prng, IRM* const irm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters);
void from_txt(std::mt19937* prng, HIRM* const irm,
              const std::string& path_schema, const std::string& path_obs,
              const std::string& path_clusters);

T_observations merge_observations(const T_observations& obs1,
                                  const T_observations& obs2);

// Return the log probability of the observations given the model h_irm.
double logp(std::mt19937* prng, std::variant<IRM*, HIRM*> h_irm,
            const T_encoding& encoding,
            const T_observations& observations);

