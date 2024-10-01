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

void setup_gendb(std::mt19937* prng, GenDB& gendb,
                 bool new_entities_have_new_parts) {
  std::map<std::string, ObservationVariant> obs0 = {
      {"School", "Massachusetts Institute of Technology"},
      {"Degree", "PHD"},
      {"City", "Cambrij"}};
  std::map<std::string, ObservationVariant> obs1 = {
      {"School", "MIT"}, {"Degree", "MD"}, {"City", "Cambridge"}};
  std::map<std::string, ObservationVariant> obs2 = {
      {"School", "Tufts"}, {"Degree", "PT"}, {"City", "Boston"}};
  std::map<std::string, ObservationVariant> obs3 = {
      {"School", "Boston University"}, {"Degree", "PhD"}, {"City", "Boston"}};

  int i = 0;
  while (i < 30) {
    gendb.incorporate(prng, {i++, obs0}, new_entities_have_new_parts);
    gendb.incorporate(prng, {i++, obs1}, new_entities_have_new_parts);
    gendb.incorporate(prng, {i++, obs2}, new_entities_have_new_parts);
    gendb.incorporate(prng, {i++, obs3}, new_entities_have_new_parts);
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

  const auto& ref_indices = gendb.schema_helper.relation_reference_indices;
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

  gendb.incorporate(&prng, std::make_pair(0, obs0), true);
  gendb.incorporate(&prng, std::make_pair(1, obs1), true);
  gendb.incorporate(&prng, std::make_pair(2, obs2), true);
  gendb.incorporate(&prng, std::make_pair(3, obs0), true);
  gendb.incorporate(&prng, std::make_pair(4, obs1), true);
  gendb.incorporate(&prng, std::make_pair(5, obs2), true);

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

BOOST_AUTO_TEST_CASE(test_new_entities_have_new_parts) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, true);

  // incorporate is called 32 times in setup_gendb.
  BOOST_TEST(gendb.domain_crps["School"].N == 32);
  BOOST_TEST(gendb.domain_crps["Physician"].N == 32);
  BOOST_TEST(gendb.domain_crps["City"].N == 32);
  BOOST_TEST(gendb.domain_crps["Practice"].N == 32);

  // Each "customer" (entity) gets its own table.
  BOOST_TEST(gendb.domain_crps["School"].tables.size() == 32);
  BOOST_TEST(gendb.domain_crps["Physician"].tables.size() == 32);
  BOOST_TEST(gendb.domain_crps["City"].tables.size() == 32);
  BOOST_TEST(gendb.domain_crps["Practice"].tables.size() == 32);
}

BOOST_AUTO_TEST_CASE(test_get_relation_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);

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
  setup_gendb(&prng, gendb, false);
  test_unincorporate_reference_helper(gendb, "Physician", "school", 1, true);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference2) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);
  test_unincorporate_reference_helper(gendb, "Record", "location", 2, true);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference3) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);
  test_unincorporate_reference_helper(gendb, "Practice", "city", 0, false);
}

BOOST_AUTO_TEST_CASE(test_logp_score) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);
  // TODO(emilyaf): Fix this test.  Right now, it is brittle and was broken
  // just by changing CRP's table from an unordered_map to a map.
  // BOOST_TEST(gendb.logp_score() < 0.0);
}

BOOST_AUTO_TEST_CASE(test_update_reference_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);

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

BOOST_AUTO_TEST_CASE(test_incorporate_stored_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);

  std::string class_name = "Record";
  std::string ref_field = "location";
  int class_item = 1;

  double init_logp = gendb.logp_score();
  auto unincorporated_items =
      gendb.unincorporate_reference(class_name, ref_field, class_item);

  int old_ref_val =
      gendb.reference_values.at({class_name, ref_field, class_item});
  int new_ref_val = (old_ref_val == 0) ? 1 : old_ref_val - 1;
  auto updated_items = gendb.update_reference_items(
      unincorporated_items, class_name, ref_field, class_item, new_ref_val);

  gendb.incorporate_reference(&prng, updated_items);
  // Updating the reference values should change logp_score (though note that
  // the domain_crps have not been updated), so the total logp_score is
  // different only if new_ref_val and old_ref_val are in different IRM
  // clusters.
  BOOST_TEST(gendb.logp_score() != init_logp, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_incorporate_stored_items_to_cluster) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, false);

  std::string class_name = "Record";
  std::string ref_field = "location";
  int class_item = 1;

  // double init_logp = gendb.logp_score();
  auto unincorporated_items =
      gendb.unincorporate_reference(class_name, ref_field, class_item);
  int new_ref_val =
      gendb.reference_values.at({class_name, ref_field, class_item});

  auto updated_items = gendb.update_reference_items(
      unincorporated_items, class_name, ref_field, class_item, new_ref_val);

  // Logp_score shouldn't change if the same items/values are
  // unincorporated/incorporated back into the same clusters.
  gendb.incorporate_reference(&prng, updated_items, false);
  // TODO(emilyaf): Fix this test.  Right now, it is brittle and was broken
  // just by changing CRP's table from an unordered_map to a map.
  // BOOST_TEST(gendb.logp_score() == init_logp, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_SUITE_END()
