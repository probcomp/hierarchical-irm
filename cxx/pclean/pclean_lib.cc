// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cctype>
#include <cstdlib>

#include "irm.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"

T_observations translate_observations(
    const DataFrame& df, const T_schema &schema,
    const std::map<std::string, std::vector<std::string>>
    &annotated_domains_for_relations) {
  T_observations obs;

  for (const auto& col : df.data) {
    const std::string& col_name = col.first;
    if (!schema.contains(col_name)) {
      printf("Schema does not contain %s, skipping ...\n", col_name.c_str());
      continue;
    }

    const T_relation& trel = schema.at(col_name);
    size_t num_domains = std::visit([&](const auto &r) {
      return r.domains.size();}, trel);

    for (size_t i = 0; i < col.second.size(); ++i) {
      const std::string& val = col.second[i];
      if (val.empty()) {
        // Don't incorporate missing values.
        // TODO(thomaswc): Allow the user to specify other values that mean
        // missing data.  ("missing", "NA", "nan", etc.).
        continue;
      }

      // Don't allow non-printable characters in val.
      for (const char c: val) {
        if (!std::isprint(c)) {
          printf("Found non-printable character with ascii value %d on line "
                 "%ld of column %s in value `%s`.\n",
                 (int)c, i+2, col_name.c_str(), val.c_str());
          std::exit(1);
        }
      }
      std::vector<std::string> entities;
      for (size_t j = 0; j < num_domains; ++j) {
        // Give every row it's own universe of unique id's.
        // TODO(thomaswc): Discuss other options for handling this, such
        // as sampling the non-index domains from a CRP prior or specifying
        // additional CSV columns to use as foreign keys.
        entities.push_back(annotated_domains_for_relations[col_name][j]
                           + ":" + std::to_string(i));
      }
      obs[col_name].push_back(std::make_tuple(entities, val));
    }
  }
  return obs;
}
