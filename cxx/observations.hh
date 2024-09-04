// Copyright 2024
// See LICENSE.txt

#pragma once

#include <map>
#include <tuple>
#include <string>
#include <vector>

// A T_observation is a <list_of_entity_ids, value>.  It does not store the
// relation it an observation of.  The i-th entity of the observation should
// belong to the i-th domain of the relation, as specified in the schema.  The
// value is stored as a string; you can use Relation::from_string to convert it
// to the appropriate ValueType.
typedef std::tuple<std::vector<std::string>, std::string> T_observation;

// T_observations[relation_name] stores the T_observation's for that relation.
typedef std::unordered_map<std::string, std::vector<T_observation>> T_observations;

// Like a T_observation, but the entity_id's have been encoded down into
// int's.
typedef std::tuple<std::vector<int>, std::string> T_encoded_observation;

// T_encoded_observations[relation_name] stores the T_encoded_observation's
// for that relation.
typedef std::unordered_map<std::string, std::vector<T_encoded_observation>> T_encoded_observations;
