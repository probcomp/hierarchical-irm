// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test GenDB

#include "gendb.hh"

#include <boost/test/included/unit_test.hpp>
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

void setup_gendb(std::mt19937* prng, GenDB& gendb) {
  std::map<std::string, ObservationVariant> obs0 = {
      {"School", "MIT"}, {"Degree", "PHD"}, {"City", "Cambrij"}};
  std::map<std::string, ObservationVariant> obs1 = {
      {"School", "MIT"}, {"Degree", "MD"}, {"City", "Cambridge"}};
  std::map<std::string, ObservationVariant> obs2 = {
      {"School", "Tufts"}, {"Degree", "PT"}, {"City", "Boston"}};

  int i = 0;
  while (i < 30) {
    gendb.incorporate(prng, {i++, obs0});
    gendb.incorporate(prng, {i++, obs1});
    gendb.incorporate(prng, {i++, obs2});
  }
}

// This tests unincorporating the current value of `class_name.ref_field`
// at index `class_item`.
void test_unincorporate_reference_helper(GenDB& gendb,
                                         const std::string& class_name,
                                         const std::string& ref_field,
                                         const int class_item,
                                         const bool from_cluster_only) {
  BOOST_TEST(
      gendb.reference_values.contains({class_name, ref_field, class_item}));

  // Store the entity of the reference class that class_name.ref_field points
  // to at class_item.
  int ref_item = gendb.reference_values.at({class_name, ref_field, class_item});

  auto unincorporated_items = gendb.unincorporate_reference(
      class_name, ref_field, class_item, from_cluster_only);

  const auto& ref_indices = gendb.relation_reference_indices;
  for (const auto& [name, trel] : gendb.hirm->schema) {
    // Store the indices of the relation domains that refer to the class and
    // the reference class.
    std::vector<std::string> domains =
        std::visit([](auto t) { return t.domains; }, trel);
    std::vector<int> ref_field_inds;
    std::vector<int> class_inds;
    for (size_t i = 0; i < domains.size(); ++i) {
      if (domains[i] == class_name) {
        bool should_check = ref_indices.contains(name) &&
                            ref_indices.at(name).at(i).contains(ref_field);
        if (should_check) {
          class_inds.push_back(i);
          ref_field_inds.push_back(ref_indices.at(name).at(i).at(ref_field));
        }
      }
    }

    // If this relation doesn't contain the class and its reference class,
    // continue.
    if (ref_field_inds.size() > 0) {
      BOOST_TEST(unincorporated_items.contains(name));
    } else {
      continue;
    }

    // Check that all of the unincorporated items have the right primary and
    // foreign key values.
    for (auto [it, unused_val] : unincorporated_items[name]) {
      bool seen_ref_value = false;
      for (size_t j = 0; j < ref_field_inds.size(); ++j) {
        seen_ref_value = seen_ref_value || (it[class_inds[j]] == class_item &&
                                            it[ref_field_inds[j]] == ref_item);
      }
      BOOST_TEST(seen_ref_value);
    }

    // Unincorporating from clusters only leaves the relation's data as-is, so
    // we stop here.
    if (from_cluster_only) {
      return;
    }

    // Check that the relation's data doesn't contain any items with the
    // primary/ foreign key values we unincorporated.
    auto check_data = [&](auto rel) {
      auto data = rel->get_data();
      for (auto [items, unused_val] : data) {
        bool seen_ref_value = false;
        for (size_t j = 0; j < ref_field_inds.size(); ++j) {
          seen_ref_value =
              seen_ref_value || (items[class_inds[j]] == class_item &&
                                 items[ref_field_inds[j]] == ref_item);
        }
        BOOST_TEST(!seen_ref_value);
      }
    };
    std::visit(check_data, gendb.hirm->get_relation(name));
  }
}

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
  BOOST_TEST(gendb.reference_values.contains({"Record", "physician", 0}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "physician", 1}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "physician", 2}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "physician", 3}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "physician", 4}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "location", 0}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "location", 1}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "location", 2}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "location", 3}));
  BOOST_TEST(gendb.reference_values.contains({"Record", "location", 4}));
  BOOST_TEST(gendb.reference_values.contains({"Physician", "school", 0}));
  BOOST_TEST(gendb.reference_values.contains({"Practice", "city", 0}));

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
        gendb.reference_values.at({"Record", "location", index});
    BOOST_TEST(expected_location == i[1]);
    int expected_city =
        gendb.reference_values.at({"Practice", "city", expected_location});
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
        gendb.reference_values.at({"Physician", "school", index});
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
        gendb.reference_values.at({"Physician", "school", index});
    BOOST_TEST(expected_school == i[0]);
  }

  std::unordered_set<T_items, H_items> cn_items =
      std::visit(get_relation_items, gendb.hirm->get_relation("City:name"));
  for (T_items i : cn_items) {
    // Domains are City
    BOOST_TEST(i.size() == 1);
  }
}

BOOST_AUTO_TEST_CASE(test_get_relation_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);

  // Each vector of items in a relation's data is entirely determined by
  // its last value (the primary key of the class lowest in the hierarchy).
  // Check that the incorporated data is as expected.
  for (const auto& [name, trel] : gendb.hirm->schema) {
    auto test_relation = [&](auto rel) {
      auto data = rel->get_data();
      for (auto [items, unused_value] : data) {
        size_t num_domains = rel->get_domains().size();
        T_items expected_items(num_domains);
        gendb.get_relation_items(name, num_domains - 1, items.back(),
                                 expected_items);
        BOOST_TEST(items.size() == expected_items.size());
        for (size_t i = 0; i < num_domains; ++i) {
          BOOST_TEST(items[i] == expected_items[i]);
        }
      }
    };
    std::visit(test_relation, gendb.hirm->get_relation(name));
  }
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference1) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  test_unincorporate_reference_helper(gendb, "Physician", "school", 1, true);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference2) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  test_unincorporate_reference_helper(gendb, "Record", "location", 2, true);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference3) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  test_unincorporate_reference_helper(gendb, "Practice", "city", 0, false);
}

BOOST_AUTO_TEST_CASE(test_update_reference_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);

  std::string class_name = "Practice";
  std::string ref_field = "city";
  int class_item = 1;

  BOOST_TEST(
      gendb.reference_values.contains({class_name, ref_field, class_item}));

  auto unincorporated_items =
      gendb.unincorporate_reference(class_name, ref_field, class_item);
  BOOST_TEST(unincorporated_items.size() > 0);

  int new_ref_val = 7;
  auto updated_items = gendb.update_reference_items(
      unincorporated_items, class_name, ref_field, class_item, new_ref_val);

  BOOST_TEST(unincorporated_items.size() == updated_items.size());
  // All of the items should have been updated.
  for (const auto& [rel_name, vals] : updated_items) {
    BOOST_TEST(vals.size() == unincorporated_items.at(rel_name).size());
    for (const auto& [items, v] : vals) {
      BOOST_TEST(!unincorporated_items.at(rel_name).contains(items));
    }
  }
}

BOOST_AUTO_TEST_CASE(test_domains_cache) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);

  std::vector<std::string> expected_domains = {"School"};
  BOOST_TEST(gendb.domains["School"] == expected_domains);

  expected_domains = {"School", "Physician"};
  BOOST_TEST(gendb.domains["Physician"] == expected_domains);

  expected_domains = {"City"};
  BOOST_TEST(gendb.domains["City"] == expected_domains);

  expected_domains = {"City", "Practice"};
  BOOST_TEST(gendb.domains["Practice"] == expected_domains);

  expected_domains = {"City", "Practice", "School", "Physician", "Record"};
  BOOST_TEST(gendb.domains["Record"] == expected_domains, tt::per_element());

  auto& ref_indices = gendb.class_reference_indices;

  // The Practice, Physician, and Record classes have reference fields, so they
  // should be included in the reference field index map.
  BOOST_TEST(ref_indices.size() == 3);

  // For Physician and Practice, index 1 corresponds to the class itself, and
  // index 0 corresponds to the reference class.
  BOOST_TEST_REQUIRE(ref_indices.contains("Physician"));
  BOOST_TEST(ref_indices.at("Physician").at(1).at("school") == 0);
  BOOST_TEST(ref_indices.at("Practice").at(1).at("city") == 0);

  // For Record, index 4 corresponds to the class itself, which points to
  // physician (index 3) and location (index 1).
  BOOST_TEST_REQUIRE(ref_indices.contains("Record"));
  BOOST_TEST(ref_indices.at("Record").at(4).at("physician") == 3);
  BOOST_TEST(ref_indices.at("Record").at(4).at("location") == 1);
  BOOST_TEST(ref_indices.at("Record").at(3).at("school") == 2);
  BOOST_TEST(ref_indices.at("Record").at(1).at("city") == 0);
}

BOOST_AUTO_TEST_CASE(test_domains_and_reference_cache_two_paths_same_source) {
  std::stringstream ss(R"""(
class City
  name ~ string

class Person
  birth_city ~ City
  home_city ~ City
)""");
  PCleanSchema schema;
  [[maybe_unused]] bool ok = read_schema(ss, &schema);
  assert(ok);
  std::mt19937 prng;
  GenDB gendb(&prng, schema);

  std::vector<std::string> expected_domains = {"City", "City", "Person"};
  BOOST_TEST(gendb.domains["Person"] == expected_domains, tt::per_element());

  auto& ref_indices = gendb.class_reference_indices;

  // Only the Person field has reference fields.
  BOOST_TEST(ref_indices.size() == 1);
  BOOST_TEST_REQUIRE(ref_indices.contains("Person"));
  BOOST_TEST(ref_indices.at("Person").at(2).at("birth_city") == 0);
  BOOST_TEST(ref_indices.at("Person").at(2).at("home_city") == 1);
}

BOOST_AUTO_TEST_CASE(test_domains_and_reference_cache_diamond) {
  std::stringstream ss(R"""(
class City
  name ~ string

class School
  location ~ City

class Practice
  location ~ City

class Physician
  practice ~ Practice
  school ~ School
)""");
  PCleanSchema schema;
  [[maybe_unused]] bool ok = read_schema(ss, &schema);
  assert(ok);
  std::mt19937 prng;
  GenDB gendb(&prng, schema);

  std::vector<std::string> expected_domains = {"City", "Practice", "City",
                                               "School", "Physician"};
  BOOST_TEST(gendb.domains["Physician"] == expected_domains,
             tt::per_element());

  auto& ref_indices = gendb.class_reference_indices;

  BOOST_TEST(ref_indices.size() == 3);

  // Physician (index 4) has a reference field "practice", which appears
  // at index 1. Practice has a reference field "location", which appears
  // at index 0.
  BOOST_TEST(ref_indices.at("Physician").at(4).at("practice") == 1);
  BOOST_TEST(ref_indices.at("Physician").at(1).at("location") == 0);

  // Physician (index 4) has a reference field "school", which appears
  // at index 3. School has a reference field "location", which appears
  // at index 2.
  BOOST_TEST(ref_indices.at("Physician").at(4).at("school") == 3);
  BOOST_TEST(ref_indices.at("Physician").at(3).at("location") == 2);

  BOOST_TEST(ref_indices.at("Practice").at(1).at("location") == 0);
  BOOST_TEST(ref_indices.at("School").at(1).at("location") == 0);
}

BOOST_AUTO_TEST_CASE(test_make_relations_for_queryfield) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  T_schema tschema;

  PCleanClass query_class = schema.classes[schema.query.record_class];
  gendb.make_relations_for_queryfield(schema.query.fields["School"],
                                              query_class, &tschema);

  BOOST_TEST(tschema.size() == 2);
  BOOST_TEST(tschema.contains("School"));
  BOOST_TEST(tschema.contains("Physician::School"));
  BOOST_TEST(std::get<T_noisy_relation>(tschema["School"]).is_observed);
  BOOST_TEST(
      !std::get<T_noisy_relation>(tschema["Physician::School"]).is_observed);
}

BOOST_AUTO_TEST_CASE(test_make_relations_for_queryfield_only_final_emissions) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema, true);
  T_schema tschema;

  PCleanClass query_class = schema.classes[schema.query.record_class];
  gendb.make_relations_for_queryfield(schema.query.fields["School"],
                                              query_class, &tschema);
  BOOST_TEST(tschema.size() == 1);
  BOOST_TEST(tschema.contains("School"));
}

BOOST_AUTO_TEST_CASE(test_make_hirm_schmea) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  T_schema tschema = gendb.make_hirm_schema();

  BOOST_TEST(tschema.contains("School:name"));
  T_clean_relation cr = std::get<T_clean_relation>(tschema["School:name"]);
  BOOST_TEST(!cr.is_observed);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::bigram));
  std::vector<std::string> expected_domains = {"School"};
  BOOST_TEST(cr.domains == expected_domains);

  BOOST_TEST(tschema.contains("School:degree_dist"));
  T_clean_relation cr2 =
      std::get<T_clean_relation>(tschema["School:degree_dist"]);
  BOOST_TEST(
      (cr2.distribution_spec.distribution == DistributionEnum::categorical));
  BOOST_TEST(cr2.distribution_spec.distribution_args.contains("k"));
  BOOST_TEST(cr2.domains == expected_domains);

  BOOST_TEST(tschema.contains("Physician:degree"));
  T_clean_relation cr3 =
      std::get<T_clean_relation>(tschema["Physician:degree"]);
  BOOST_TEST(
      (cr3.distribution_spec.distribution == DistributionEnum::stringcat));
  std::vector<std::string> expected_domains2 = {"School", "Physician"};
  BOOST_TEST(cr3.domains == expected_domains2);

  BOOST_TEST(tschema.contains("Physician:specialty"));

  BOOST_TEST(tschema.contains("City:name"));
  T_clean_relation cr4 = std::get<T_clean_relation>(tschema["City:name"]);
  std::vector<std::string> expected_domains3 = {"City"};
  BOOST_TEST(cr4.domains == expected_domains3);

  BOOST_TEST(tschema.contains("City:state"));

  BOOST_TEST(tschema.contains("Specialty"));
  T_noisy_relation nr1 = std::get<T_noisy_relation>(tschema["Specialty"]);
  BOOST_TEST(nr1.is_observed);
  BOOST_TEST((nr1.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr1.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("School"));
  T_noisy_relation nr2 = std::get<T_noisy_relation>(tschema["School"]);
  BOOST_TEST(nr2.is_observed);
  BOOST_TEST((nr2.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr2.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("Degree"));
  T_noisy_relation nr3 = std::get<T_noisy_relation>(tschema["Degree"]);
  BOOST_TEST(nr3.is_observed);
  BOOST_TEST((nr3.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr3.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("City"));
  T_noisy_relation nr4 = std::get<T_noisy_relation>(tschema["City"]);
  BOOST_TEST(nr4.is_observed);
  BOOST_TEST((nr4.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"City", "Practice", "Record"};
  BOOST_TEST(nr4.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("State"));
  T_noisy_relation nr5 = std::get<T_noisy_relation>(tschema["State"]);
  BOOST_TEST(nr5.is_observed);
  BOOST_TEST((nr5.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"City", "Practice", "Record"};
  BOOST_TEST(nr5.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("Physician::School"));
  T_noisy_relation nr6 =
      std::get<T_noisy_relation>(tschema["Physician::School"]);
  BOOST_TEST(!nr6.is_observed);
  expected_domains = {"School", "Physician"};
  BOOST_TEST(nr6.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("Practice::City"));
  T_noisy_relation nr7 = std::get<T_noisy_relation>(tschema["Practice::City"]);
  BOOST_TEST(!nr7.is_observed);
  expected_domains = {"City", "Practice"};
  BOOST_TEST(nr7.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("Practice::State"));
  T_noisy_relation nr8 = std::get<T_noisy_relation>(tschema["Practice::State"]);
  BOOST_TEST(!nr8.is_observed);
  expected_domains = {"City", "Practice"};
  BOOST_TEST(nr8.domains == expected_domains, tt::per_element());

  auto& ref_indices = gendb.relation_reference_indices;

  // Practice (index 1) has a reference field "city", which appears
  // at index 0.
  BOOST_TEST(ref_indices.at("Practice::State").at(1).at("city") == 0);

  // Record (index 2) has a reference field "location", which appears
  // at index 1 (and refers to Practice). Practice has a reference field
  // "city", which appears at index 0.
  BOOST_TEST(ref_indices.at("State").at(2).at("location") == 1);
  BOOST_TEST(ref_indices.at("State").at(1).at("city") == 0);
}

BOOST_AUTO_TEST_CASE(test_make_hirm_schema_only_final_emissions) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema, true);
  T_schema tschema = gendb.make_hirm_schema();

  BOOST_TEST(tschema.contains("School:name"));
  T_clean_relation cr = std::get<T_clean_relation>(tschema["School:name"]);
  BOOST_TEST(!cr.is_observed);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::bigram));
  std::vector<std::string> expected_domains = {"School"};
  BOOST_TEST(cr.domains == expected_domains);

  BOOST_TEST(tschema.contains("School:degree_dist"));
  T_clean_relation cr2 =
      std::get<T_clean_relation>(tschema["School:degree_dist"]);
  BOOST_TEST(
      (cr2.distribution_spec.distribution == DistributionEnum::categorical));
  BOOST_TEST(cr2.distribution_spec.distribution_args.contains("k"));
  BOOST_TEST(cr2.domains == expected_domains);

  BOOST_TEST(tschema.contains("Physician:degree"));
  T_clean_relation cr3 =
      std::get<T_clean_relation>(tschema["Physician:degree"]);
  BOOST_TEST(
      (cr3.distribution_spec.distribution == DistributionEnum::stringcat));
  std::vector<std::string> expected_domains2 = {"School", "Physician"};
  BOOST_TEST(cr3.domains == expected_domains2);

  BOOST_TEST(tschema.contains("Physician:specialty"));

  BOOST_TEST(tschema.contains("City:name"));
  T_clean_relation cr4 = std::get<T_clean_relation>(tschema["City:name"]);
  std::vector<std::string> expected_domains3 = {"City"};
  BOOST_TEST(cr4.domains == expected_domains3);

  BOOST_TEST(tschema.contains("City:state"));

  BOOST_TEST(tschema.contains("Specialty"));
  T_noisy_relation nr1 = std::get<T_noisy_relation>(tschema["Specialty"]);
  BOOST_TEST(nr1.is_observed);
  BOOST_TEST((nr1.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr1.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("School"));
  T_noisy_relation nr2 = std::get<T_noisy_relation>(tschema["School"]);
  BOOST_TEST(nr2.is_observed);
  BOOST_TEST((nr2.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr2.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("Degree"));
  T_noisy_relation nr3 = std::get<T_noisy_relation>(tschema["Degree"]);
  BOOST_TEST(nr3.is_observed);
  BOOST_TEST((nr3.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"School", "Physician", "Record"};
  BOOST_TEST(nr3.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("City"));
  T_noisy_relation nr4 = std::get<T_noisy_relation>(tschema["City"]);
  BOOST_TEST(nr4.is_observed);
  BOOST_TEST((nr4.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"City", "Practice", "Record"};
  BOOST_TEST(nr4.domains == expected_domains, tt::per_element());

  BOOST_TEST(tschema.contains("State"));
  T_noisy_relation nr5 = std::get<T_noisy_relation>(tschema["State"]);
  BOOST_TEST(nr5.is_observed);
  BOOST_TEST((nr5.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {"City", "Practice", "Record"};
  BOOST_TEST(nr5.domains == expected_domains, tt::per_element());

  auto& ref_indices = gendb.relation_reference_indices;
  BOOST_TEST(ref_indices.at("State").at(2).at("location") == 1);
  BOOST_TEST(ref_indices.at("State").at(1).at("city") == 0);
}

BOOST_AUTO_TEST_CASE(test_record_class_is_clean) {
  std::stringstream ss2(R"""(
class Record
  rent ~ real

observe
  rent as "Rent"
  from Record
)""");
  PCleanSchema schema2;
  [[maybe_unused]] bool ok = read_schema(ss2, &schema2);
  assert(ok);

  std::mt19937 prng;
  GenDB gendb(&prng, schema2, false, true);
  T_schema tschema = gendb.make_hirm_schema();

  BOOST_TEST(!tschema.contains("Record:rent"));
  BOOST_TEST(tschema.contains("Rent"));

  T_clean_relation cr = std::get<T_clean_relation>(tschema["Rent"]);
  BOOST_TEST(cr.is_observed);
}

BOOST_AUTO_TEST_CASE(test_record_class_is_dirty) {
  std::stringstream ss2(R"""(
class Record
  rent ~ real

observe
  rent as "Rent"
  from Record
)""");
  PCleanSchema schema2;
  [[maybe_unused]] bool ok = read_schema(ss2, &schema2);
  assert(ok);

  std::mt19937 prng;
  GenDB gendb(&prng, schema2, false, false);
  T_schema tschema = gendb.make_hirm_schema();

  BOOST_TEST(tschema.contains("Record:rent"));
  BOOST_TEST(tschema.contains("Rent"));

  T_clean_relation cr = std::get<T_clean_relation>(tschema["Record:rent"]);
  BOOST_TEST(!cr.is_observed);
  T_noisy_relation nr = std::get<T_noisy_relation>(tschema["Rent"]);
  BOOST_TEST(nr.is_observed);
}

BOOST_AUTO_TEST_SUITE_END()
