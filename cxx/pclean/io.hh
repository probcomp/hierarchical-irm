// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <iostream>
#include <string>
#include "pclean/schema.hh"

// Reads the schema file at pathname path into *schema.
// Returns true on success, false on failure.
bool read_schema_file(const std::string& path, PCleanSchema* schema);

// Reads the schema from the input stream.
bool read_schema(std::istream& is, PCleanSchema* schema);
