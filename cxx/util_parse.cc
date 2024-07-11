// Copyright 2024
// See LICENSE.txt

#include <cassert>
#include <map>
#include <string>

#include "util_parse.hh"

void parse_name_and_parameters(
    const std::string& name_and_params,
    std::string* name,
    std::map<std::string, std::string>* params) {
  size_t pos = name_and_params.find('(');
  if (pos == std::string::npos) {
    *name = name_and_params;
    return;
  }

  *name = name_and_params.substr(0, pos);
  assert(name_and_params.back() == ')');

  std::string rest = name_and_params.substr(pos + 1);
  rest.pop_back();  // Remove )

  while (rest.length() > 0)  {
    std::string current;
    pos = rest.find(',');
    if (pos == std::string::npos) {
      current = rest;
      rest = "";
    } else {
      current = rest.substr(0, pos);
      rest = rest.substr(pos + 1);
    }
    pos = current.find('=');
    if (pos == std::string::npos) {
      printf("Warning: invalid parameter piece %s in %s\n",
             current.c_str(), name_and_params.c_str());
      continue;
    }
    size_t arg_start = 0;
    while (current[arg_start] == ' ') {
      arg_start++;
    }
    std::string arg_name = current.substr(arg_start, pos - arg_start);
    std::string arg_value = current.substr(pos + 1);
    (*params)[arg_name] = arg_value;
  }
}

