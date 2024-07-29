#include "pclean/csv.hh"

#include <cassert>
#include <fstream>
#include <sstream>

DataFrame DataFrame::from_csv(
    const std::string& filename, const std::vector<std::string>& column_names) {
  std::ifstream f(filename);

  if (!f.is_open()) {
    printf("Unable to open CSV file %s\n", filename.c_str());
    assert(false);
  }

  DataFrame df = DataFrame::from_csv(f, column_names);
  f.close();

  return df;
}

DataFrame DataFrame::from_csv(
    std::istream& is, const std::vector<std::string>& column_names) {
  DataFrame df;
  std::vector<std::string> col_names;

  if (!column_names.empty()) {
    col_names = column_names;
    for (const auto& c : column_names) {
      df.data[c] = {};
    }
  }

  bool first_line = true;
  std::string line;
  while (std::getline(is, line)) {
    // TODO(thomawc): Strip off comments started with a #
    if (line.empty()) {
      continue;
    }

    std::stringstream ss(line);
    std::string part;
    size_t i = 0;
    // TODO(thomaswc): Handle quoted fields
    while (std::getline(ss, part, ',')) {
      if (first_line && column_names.empty()) {
        col_names.push_back(part);
        df.data[part] = {};
        continue;
      }

      df.data[col_names[i++]].push_back(part);
    }
    if (!first_line) {
      assert(i == col_names.size());
    }
    first_line = false;
  }

  return df;
}
