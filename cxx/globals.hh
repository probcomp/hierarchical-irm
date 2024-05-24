// Copyright 2021 MIT Probabilistic Computing Project
// Apache License, Version 2.0, refer to LICENSE.txt

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::map;
using std::string;
using std::tuple;
using std::vector;

#define uset std::unordered_set
#define umap std::unordered_map

// https://stackoverflow.com/q/2241327/
typedef std::mt19937 PRNG;

// T_relation is the text we get from reading a line of the schema file;
// hirm.hh:Relation is the object that does the work.
class T_relation {
 public:
  // The relation is a map from the domains to the space .distribution
  // is a distribution over.
  vector<string> domains;

  // Must be the name of a distribution in distributions/.
  string distribution;
};

// Map from names to T_relation's.
typedef map<string, T_relation> T_schema;

extern const double INF;
