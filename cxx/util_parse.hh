// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <string>

// Parse a string like "name" or "name(param1=value1, param2=value2)" into
// its name and parameters.
void parse_name_and_parameters(
    const std::string& name_and_params,
    std::string* name,
    std::map<std::string, std::string>* params);
