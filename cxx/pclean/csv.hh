// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

// Holds a two-dimensional table with named columns.
class DataFrame {
 public:
  DataFrame() {}

  ~DataFrame() {}

  // Reads a DataFrame from a comma-separated value file.
  // If column_names is empty, the column names are extracted from the first
  // line of the file.
  static DataFrame from_csv(const std::string& filename,
                            const std::vector<std::string>& column_names = {});
  static DataFrame from_csv(std::istream& is,
                            const std::vector<std::string>& column_names = {});

  // Writes the DataFrame to a file.  Returns false on error.
  bool to_csv(const std::string& filename);
  bool to_csv(std::ostream& os);

  std::vector<std::string> columns;

  // data['column_name'] holds the data for that column.
  std::map<std::string, std::vector<std::string>> data;
};
