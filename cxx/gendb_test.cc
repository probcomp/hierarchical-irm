// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test GenDB

#include "gendb.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>
#include <iostream>

#include "pclean/io.hh"

namespace tt = boost::test_tools;

struct SchemaTestFixture {
  SchemaTestFixture() {
    std::stringstream ss(R"""(
class School
  name ~ real

class Physician
  school ~ School
  degree ~ stringcat(strings="MD PT NP DO PHD")

class City
  name ~ real

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

void setup_gendb(std::mt19937* prng, GenDB& gendb, int n = 30) {
  std::normal_distribution<double> d(0., 1.);
  std::map<std::string, ObservationVariant> obs0 = {
      {"School", d(*prng)}, {"Degree", "PHD"}, {"City", d(*prng)}};
  std::map<std::string, ObservationVariant> obs1 = {
      {"School", d(*prng)}, {"Degree", "MD"}, {"City", d(*prng)}};
  std::map<std::string, ObservationVariant> obs2 = {
      {"School", d(*prng)}, {"Degree", "PT"}, {"City", d(*prng)}};
  std::map<std::string, ObservationVariant> obs3 = {
      {"School", d(*prng)}, {"Degree", "PhD"}, {"City", d(*prng)}};

  int i = 0;
  while (i < n) {
    gendb.incorporate(prng, {i++, obs0});
    gendb.incorporate(prng, {i++, obs1});
    gendb.incorporate(prng, {i++, obs2});
    gendb.incorporate(prng, {i++, obs3});
  }
}

// This tests unincorporating the current value of `class_name.ref_field`
// at index `class_item`.
void test_unincorporate_reference_helper(GenDB& gendb,
                                         const std::string& class_name,
                                         const std::string& ref_field,
                                         const int class_item) {
  BOOST_TEST(
      gendb.reference_values.at(class_name).contains({ref_field, class_item}));

  // Store the entity of the reference class that class_name.ref_field points
  // to at class_item.
  int ref_item =
      gendb.reference_values.at(class_name).at({ref_field, class_item});

  std::map<std::string, std::vector<size_t>> domain_inds =
      gendb.get_domain_inds(class_name, ref_field);
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_value_map;
  std::map<std::tuple<int, std::string, T_item>, int>
      unincorporated_from_domains;
  gendb.unincorporate_reference(domain_inds, class_name, ref_field, class_item,
                                stored_value_map, unincorporated_from_domains);

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
      BOOST_TEST(stored_value_map.contains(name));
    } else {
      continue;
    }

    // Check that all of the unincorporated items have the right primary and
    // foreign key values.
    for (auto [it, unused_val] : stored_value_map[name]) {
      bool seen_ref_value = false;
      for (size_t j = 0; j < ref_field_inds.size(); ++j) {
        seen_ref_value = seen_ref_value || (it[class_inds[j]] == class_item &&
                                            it[ref_field_inds[j]] == ref_item);
      }
      BOOST_TEST(seen_ref_value);
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

  std::normal_distribution<double> d(0., 1.);
  std::map<std::string, ObservationVariant> obs0 = {
      {"School", d(prng)}, {"Degree", "PHD"}, {"City", d(prng)}};
  std::map<std::string, ObservationVariant> obs1 = {
      {"School", d(prng)}, {"Degree", "MD"}, {"City", d(prng)}};
  std::map<std::string, ObservationVariant> obs2 = {
      {"School", d(prng)}, {"Degree", "PT"}, {"City", d(prng)}};

  gendb.incorporate(&prng, std::make_pair(0, obs0));
  gendb.incorporate(&prng, std::make_pair(1, obs1));
  gendb.incorporate(&prng, std::make_pair(2, obs2));
  gendb.incorporate(&prng, std::make_pair(3, obs0));
  gendb.incorporate(&prng, std::make_pair(4, obs1));
  gendb.incorporate(&prng, std::make_pair(5, obs2));

  // Check that the structure of reference_values is as expected.
  // School and City are not contained in reference_values because they
  // have no reference fields.
  BOOST_TEST(gendb.reference_values.at("Record").contains({"physician", 0}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"physician", 1}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"physician", 2}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"physician", 3}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"physician", 4}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"location", 0}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"location", 1}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"location", 2}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"location", 3}));
  BOOST_TEST(gendb.reference_values.at("Record").contains({"location", 4}));
  BOOST_TEST(gendb.reference_values.at("Physician").contains({"school", 0}));
  BOOST_TEST(gendb.reference_values.at("Practice").contains({"city", 0}));

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
        gendb.reference_values.at("Record").at({"location", index});
    BOOST_TEST(expected_location == i[1]);
    int expected_city =
        gendb.reference_values.at("Practice").at({"city", expected_location});
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
        gendb.reference_values.at("Physician").at({"school", index});
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
        gendb.reference_values.at("Physician").at({"school", index});
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
  test_unincorporate_reference_helper(gendb, "Physician", "school", 1);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference2) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  test_unincorporate_reference_helper(gendb, "Record", "location", 2);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reference3) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  test_unincorporate_reference_helper(gendb, "Practice", "city", 0);
}

BOOST_AUTO_TEST_CASE(test_logp_score) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);
  BOOST_TEST(gendb.logp_score() < 0.0);
}

BOOST_AUTO_TEST_CASE(test_update_reference_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);

  std::string class_name = "Practice";
  std::string ref_field = "city";
  int class_item = 0;

  BOOST_TEST(
      gendb.reference_values.at(class_name).contains({ref_field, class_item}));

  std::map<std::string, std::vector<size_t>> domain_inds =
      gendb.get_domain_inds(class_name, ref_field);
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_value_map;
  std::map<std::tuple<int, std::string, T_item>, int>
      unincorporated_from_domains;
  gendb.unincorporate_reference(domain_inds, class_name, ref_field, class_item,
                                stored_value_map, unincorporated_from_domains);
  BOOST_TEST(stored_value_map.size() > 0);

  int new_ref_val = 7;
  auto updated_items = gendb.update_reference_items(
      stored_value_map, class_name, ref_field, class_item, new_ref_val);

  BOOST_TEST(stored_value_map.size() == updated_items.size());
  // All of the items should have been updated.
  for (const auto& [rel_name, vals] : updated_items) {
    BOOST_TEST(vals.size() == stored_value_map.at(rel_name).size());
    for (const auto& [items, v] : vals) {
      BOOST_TEST(!stored_value_map.at(rel_name).contains(items));
    }
  }
}

BOOST_AUTO_TEST_CASE(test_incorporate_stored_items) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, 5);

  std::string class_name = "Record";
  std::string ref_field = "location";
  int class_item = 0;

  double init_logp = gendb.logp_score();
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_value_map;
  std::map<std::string, std::vector<size_t>> domain_inds =
      gendb.get_domain_inds(class_name, ref_field);
  std::map<std::tuple<int, std::string, T_item>, int>
      unincorporated_from_domains;
  gendb.unincorporate_reference(domain_inds, class_name, ref_field, class_item,
                                stored_value_map, unincorporated_from_domains);

  int old_ref_val =
      gendb.reference_values.at(class_name).at({ref_field, class_item});
  int new_ref_val = (old_ref_val == 0) ? 1 : old_ref_val - 1;
  auto updated_items = gendb.update_reference_items(
      stored_value_map, class_name, ref_field, class_item, new_ref_val);

  gendb.incorporate_reference(&prng, updated_items);
  // Updating the reference values should change logp_score (though note that
  // the domain_crps have not been updated), so the total logp_score is
  // different only if new_ref_val and old_ref_val are in different IRM
  // clusters.
  BOOST_TEST(gendb.logp_score() != init_logp, tt::tolerance(1e-6));
}

BOOST_AUTO_TEST_CASE(test_transition_reference) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb);

  std::string class_name = "Record";
  std::string ref_field = "physician";
  int class_item = 1;

  gendb.transition_reference(&prng, class_name, ref_field, class_item);
  gendb.transition_reference(&prng, class_name, ref_field, class_item);

  class_name = "Physician";
  ref_field = "school";
  class_item = 0;
  gendb.transition_reference(&prng, class_name, ref_field, class_item);
  gendb.transition_reference(&prng, class_name, ref_field, class_item);
  gendb.transition_reference(&prng, class_name, ref_field, class_item);
  gendb.transition_reference(&prng, class_name, ref_field, class_item);

  BOOST_TEST(gendb.logp_score() < 0.);
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reincorporate_existing) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, 20);

  std::string class_name = "Physician";
  std::string ref_field = "school";
  std::string ref_class = "School";

  // Find a non-singleton reference value.
  int refval = -1;
  int class_item = 0;
  int init_refval_obs;
  int candidate_refval;
  while (true) {
    candidate_refval =
        gendb.reference_values.at(class_name).at({ref_field, class_item});
    init_refval_obs =
        gendb.domain_crps.at(ref_class).tables.at(candidate_refval).size();
    if (init_refval_obs > 1) {
      refval = candidate_refval;
      break;
    }
    ++class_item;
  }
  assert(refval > -1);

  double init_logpscore = gendb.logp_score();

  // Unincorporate the reference value.
  auto domain_inds = gendb.get_domain_inds(class_name, ref_field);
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_values;
  std::map<std::tuple<int, std::string, T_item>, int>
      unincorporated_from_domains;
  std::map<std::tuple<std::string, std::string, int>, int>
      unincorporated_from_entity_crps;
  gendb.unincorporate_reference(domain_inds, class_name, ref_field, class_item,
                                stored_values, unincorporated_from_domains);
  int ref_id = gendb.get_reference_id(class_name, ref_field, class_item);
  gendb.domain_crps.at(ref_class).unincorporate(ref_id);

  // Check that the reference value was unincorporated from its entity CRP.
  BOOST_TEST(gendb.domain_crps.at(ref_class).tables.size() =
                 init_refval_obs - 1);

  // Test that the class item/reference value were unincorporated from the
  // relations. The reference value should still be in the domain CRP since it
  // wasn't a singleton.
  for (const std::string& r : gendb.class_to_relations.at(class_name)) {
    auto f = [&](auto rel) {
      for (auto& [items, val] : rel->get_data()) {
        BOOST_TEST(items.back() != class_item);
      }
    };
    std::visit(f, gendb.hirm->get_relation(r));

    IRM* irm = gendb.hirm->relation_to_irm(r);
    BOOST_TEST(irm->has_observation(ref_class, class_item));
  }

  double baseline_logp = gendb.logp_score();

  // Logp score should be unchanged after re-incorporating.
  gendb.reincorporate_new_refval(
      class_name, ref_field, class_item, refval, ref_class, stored_values,
      unincorporated_from_domains, unincorporated_from_entity_crps);
  BOOST_TEST(
      init_logpscore - baseline_logp == gendb.logp_score() - baseline_logp,
      tt::tolerance(1e-3));
}

BOOST_AUTO_TEST_CASE(test_unincorporate_reincorporate_new) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, 20);

  std::string class_name = "Practice";
  std::string ref_field = "city";
  std::string ref_class = "City";

  // Find a non-singleton reference value.
  int non_singleton_refval = -1;
  int class_item = 0;
  int candidate_refval;
  while (true) {
    candidate_refval =
        gendb.reference_values.at(class_name).at({ref_field, class_item});
    if (gendb.domain_crps.at(ref_class).tables.at(candidate_refval).size() >
        1) {
      non_singleton_refval = candidate_refval;
      break;
    }
    ++class_item;
  }
  assert(non_singleton_refval > -1);

  // Now find the singleton value.
  int refval = -1;
  std::unordered_map<int, double> crp_dist =
      gendb.domain_crps[ref_class].tables_weights_gibbs(non_singleton_refval);
  for (auto [t, w] : crp_dist) {
    if (!gendb.domain_crps[ref_class].tables.contains(t)) {
      refval = t;
    }
  }
  assert(refval != -1);

  double init_logp = gendb.logp_score();

  auto domain_inds = gendb.get_domain_inds(class_name, ref_field);
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      stored_values;
  std::map<std::tuple<int, std::string, T_item>, int>
      unincorporated_from_domains;
  std::map<std::string,
           std::unordered_map<T_items, ObservationVariant, H_items>>
      ref_class_relation_stored_values;
  std::map<std::tuple<std::string, std::string, int>, int>
      unincorporated_from_entity_crps;
  int ref_id = gendb.get_reference_id(class_name, ref_field, class_item);
  gendb.domain_crps.at(ref_class).unincorporate(ref_id);
  gendb.unincorporate_reference(domain_inds, class_name, ref_field, class_item,
                                stored_values, unincorporated_from_domains);
  gendb.unincorporate_singleton(class_name, ref_field, class_item, ref_class,
                                ref_class_relation_stored_values,
                                unincorporated_from_domains,
                                unincorporated_from_entity_crps);

  double baseline_logp = gendb.logp_score();

  // The entity CRP doesn't contain the reference value.
  BOOST_TEST(!gendb.domain_crps.at(ref_class).tables.contains(refval));

  // Test that the reference value was unincorporated from IRM domain CRPs.
  for (auto [code, irm] : gendb.hirm->irms) {
    for (auto [name, d] : irm->domains) {
      if (name == ref_class) {
        BOOST_TEST(!d->items.contains(refval));
      }
    }
  }

  gendb.reincorporate_new_refval(
      class_name, ref_field, class_item, refval, ref_class, stored_values,
      unincorporated_from_domains, unincorporated_from_entity_crps);
  // Logp score should be different.
  BOOST_TEST(gendb.logp_score() - baseline_logp != init_logp - baseline_logp,
             tt::tolerance(1e-3));
}

BOOST_AUTO_TEST_CASE(test_transition_reference_class) {
  std::mt19937 prng;
  GenDB gendb(&prng, schema);
  setup_gendb(&prng, gendb, 20);
  gendb.transition_reference_class_and_ancestors(&prng, "Record");
}

BOOST_AUTO_TEST_CASE(test_transition_reference_complex_schema) {
  std::stringstream ss_complex(R"""(
class A
  x ~ real

class B
  a ~ A

class E
  y ~ real

class C
  a ~ A
  b ~ B

class Record
  b ~ B
  c ~ C
  e ~ E

observe
  b.a.x as BAX
  c.a.x as CAX
  c.b.a.x as CBAX
  e.y as EY
  from Record
)""");
  PCleanSchema complex_schema;
  [[maybe_unused]] bool ok = read_schema(ss_complex, &complex_schema);
  assert(ok);
  
  std::mt19937 prng;
  GenDB gendb(&prng, complex_schema);

  std::normal_distribution<double> d(0., 1.);
  std::map<std::string, ObservationVariant> obs;

  int i = 0;
  while (i < 30) {
    obs = {{"BAX", d(prng)}, {"CAX", d(prng)}, {"CBAX", d(prng)}, {"EY", d(prng)}};
    gendb.incorporate(&prng, {i++, obs});
  }
  gendb.transition_reference_class_and_ancestors(&prng, "Record");

  
}

BOOST_AUTO_TEST_SUITE_END()
