// Copyright 2020
// See LICENSE.txt

#pragma once

#include "hirm.hh"

typedef std::map<std::string, std::map<std::string, T_item>> T_encoding_f;
typedef std::map<std::string, std::map<T_item, std::string>> T_encoding_r;
typedef std::tuple<T_encoding_f, T_encoding_r> T_encoding;

typedef std::tuple<std::string, std::vector<std::string>, double> T_observation;
typedef std::vector<T_observation> T_observations;

typedef std::unordered_map<std::string, T_item> T_assignment;
typedef std::unordered_map<std::string, T_assignment> T_assignments;

// disk IO
T_schema load_schema(const std::string& path);
T_observations load_observations(const std::string& path);
T_encoding encode_observations(const T_schema& schema,
                               const T_observations& observations);

void incorporate_observations(IRM& irm, const T_encoding& encoding,
                              const T_observations& observations);
void incorporate_observations(HIRM& hirm, const T_encoding& encoding,
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

void from_txt(IRM* const irm, const std::string& path_schema,
              const std::string& path_obs, const std::string& path_clusters);
void from_txt(HIRM* const irm, const std::string& path_schema,
              const std::string& path_obs, const std::string& path_clusters);
