// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test GenDB

#include "gendb.hh"

#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <random>

#include "pclean/io.hh"

namespace tt = boost::test_tools;

struct SchemaTestFixture {
  SchemaTestFixture() {
    std::stringstream ss(R"""(
class School
  name ~ string

class Physician
  school ~ School
  degree ~ stringcat(strings="MD PT NP DO PHD")

class City
  name ~ string

class Practice
  city ~ City

class Record
  physician ~ Physician
  location ~ Practice

observe
  physician.school.name as School
  physician.degree as Degree
  location.city.name as City
  from Record
)""");
    [[maybe_unused]] bool ok = read_schema(ss, &schema);
    assert(ok);
  }

  ~SchemaTestFixture() {}

  PCleanSchema schema;
};

BOOST_FIXTURE_TEST_SUITE(gendb_test_suite, SchemaTestFixture)

BOOST_AUTO_TEST_CASE(test_gendb) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);

  std::map<std::string, ObservationVariant> obs0 = {
      {"School", "MIT"}, {"Degree", "PHD"}, {"City", "Cambrij"}};
  std::map<std::string, ObservationVariant> obs1 = {
      {"School", "MIT"}, {"Degree", "MD"}, {"City", "Cambridge"}};
  std::map<std::string, ObservationVariant> obs2 = {
      {"School", "Tufts"}, {"Degree", "PT"}, {"City", "Boston"}};

  gendb.incorporate(&prng, std::make_pair(0, obs0));
  gendb.incorporate(&prng, std::make_pair(1, obs1));
  gendb.incorporate(&prng, std::make_pair(2, obs2));
  gendb.incorporate(&prng, std::make_pair(3, obs0));
  gendb.incorporate(&prng, std::make_pair(4, obs1));
  gendb.incorporate(&prng, std::make_pair(5, obs2));

  // Check that the structure of reference_values is as expected.
  // School and City are not contained in reference_values because they
  // have no reference fields.
  BOOST_TEST(gendb.reference_values.size() == 3);
  BOOST_TEST(gendb.reference_values.contains("Physician"));
  BOOST_TEST(gendb.reference_values.contains("Practice"));
  BOOST_TEST(gendb.reference_values.contains("Record"));

  BOOST_TEST(gendb.reference_values.at("Physician").at(0).size() == 1);
  BOOST_TEST(gendb.reference_values.at("Physician").at(0).contains("school"));
  BOOST_TEST(gendb.reference_values.at("Practice").at(0).size() == 1);
  BOOST_TEST(gendb.reference_values.at("Practice").at(0).contains("city"));
  BOOST_TEST(gendb.reference_values.at("Record").at(0).size() == 2);
  BOOST_TEST(gendb.reference_values.at("Record").at(0).contains("physician"));
  BOOST_TEST(gendb.reference_values.at("Record").at(0).contains("location"));

  auto get_relation_items = [&](auto rel) {
    std::unordered_set<T_items, H_items> items;
    auto data = rel->get_data();
    for (auto [i, val] : data) {
      items.insert(i);
    }
    return items;
  };

  // Query relation items are as expected.
  std::unordered_set<T_items, H_items> city_items =
      std::visit(get_relation_items, gendb.hirm->get_relation("City"));
  for (T_items i : city_items) {
    // Domains are City, Practice, Record
    BOOST_TEST(i.size() == 3);
    int index = i[2];
    int expected_location =
        gendb.reference_values.at("Record").at(index).at("location");
    BOOST_TEST(expected_location == i[1]);
    int expected_city =
        gendb.reference_values.at("Practice").at(expected_location).at("city");
    BOOST_TEST(expected_city == i[0]);
  }

  // Intermediate noisy relation items are as expected.
  std::unordered_set<T_items, H_items> ps_items = std::visit(
      get_relation_items, gendb.hirm->get_relation("Physician::School"));
  for (T_items i : ps_items) {
    // Domains are School, Physician
    BOOST_TEST(i.size() == 2);
    int index = i[1];
    int expected_school =
        gendb.reference_values.at("Physician").at(index).at("school");
    BOOST_TEST(expected_school == i[0]);
  }

  // Clean relation items are as expected.
  std::unordered_set<T_items, H_items> pd_items = std::visit(
      get_relation_items, gendb.hirm->get_relation("Physician:degree"));
  for (T_items i : pd_items) {
    // Domains are Physician, School
    BOOST_TEST(i.size() == 2);
    int index = i[1];
    int expected_school =
        gendb.reference_values.at("Physician").at(index).at("school");
    BOOST_TEST(expected_school == i[0]);
  }

  std::unordered_set<T_items, H_items> cn_items =
      std::visit(get_relation_items, gendb.hirm->get_relation("City:name"));
  for (T_items i : cn_items) {
    // Domains are City
    BOOST_TEST(i.size() == 1);
  }
}

BOOST_AUTO_TEST_SUITE_END()
