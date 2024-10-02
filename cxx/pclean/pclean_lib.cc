// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <cctype>
#include <cstdlib>

#include "irm.hh"
#include "pclean/csv.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema.hh"

void incorporate_observations(std::mt19937* prng,
                              GenDB *gendb,
                              const DataFrame& df) {
  int num_rows = df.data.begin()->second.size();
  for (int i = 0; i < num_rows; i++) {
    std::map<std::string, ObservationVariant> row_values;
    for (const auto& col : df.data) {
      const std::string& col_name = col.first;
      if (!gendb->schema.query.fields.contains(col_name)) {
        if (i == 0) {
          printf("Schema does not contain %s, skipping ...\n", col_name.c_str());
        }
        continue;
      }
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
                 "%d of column %s in value `%s`.\n",
                 (int) c, i + 2, col_name.c_str(), val.c_str());
          std::exit(1);
        }
      }

      const RelationVariant& rv = gendb->hirm->get_relation(col_name);
      ObservationVariant ov;
      std::visit([&](const auto &r) { ov = r->from_string(val); }, rv);
      row_values[col_name] = ov;
    }
    // Incorporate into the gendb with new_rows_have_unique_entities=true.
    // TODO(emilyaf): Consider using new_rows_have_unique_entities=false
    // after entity transitions are allowed and numeric stability issues
    // are addressed.
    gendb->incorporate(prng, std::make_pair(i, row_values), true);
  }
}

// Sample a single "row" into *query_values.  A value is sampled into
// (*query_values)[f] for every query field in the schema.
void make_pclean_sample(
    std::mt19937* prng, GenDB* gendb, int class_item,
    std::map<std::string, std::string> *query_values) {
  for (const auto& [name, query_field] : gendb->schema.query.fields) {
    T_items entities = gendb->sample_entities_relation(
        prng, gendb->schema.query.record_class,
        query_field.class_path.begin(), query_field.class_path.end(),
        class_item, false);

    (*query_values)[query_field.name] = gendb->hirm->sample_and_incorporate_relation(
        prng, query_field.name, entities);
  }
}

DataFrame make_pclean_samples(int num_samples, int start_row, GenDB *gendb,
                              std::mt19937* prng) {
  DataFrame df;
  for (int i = 0; i < num_samples; i++) {
     std::map<std::string, std::string> query_values;
     make_pclean_sample(prng, gendb, start_row + i, &query_values);
     for (const auto& [column, val] : query_values) {
       df.data[column].push_back(val);
     }

  }
  return df;
}

T_encoding make_dummy_encoding_from_gendb(const GenDB& gendb) {
  T_encoding_f item_to_code;
  T_encoding_r code_to_item;

  for (const auto& [domain, crp] : gendb.domain_crps) {
    for (int i = 0; i < crp.max_table(); ++i) {
      code_to_item[domain][i] = domain + ":" + std::to_string(i);
    }
  }

  return std::make_pair(item_to_code, code_to_item);
}
