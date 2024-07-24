// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include "irm.hh"
#include "pclean/schema.hh"

void translate_pclean_schema_to_hirm_schema(
    const PCleanSchema& pclean_schema,
    T_schema *hirm_schema);
