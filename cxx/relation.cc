#include "relation.hh"

template <>
std::string Relation<std::string>::from_string(const std::string& s) {
  return s;
}
