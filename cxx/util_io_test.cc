#define BOOST_TEST_MODULE test UtilIO

#include "util_io.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_nary_relation) {
  T_schema schema = load_schema("test_schema/nary.schema");
  // Check that the relations are correct.
  BOOST_TEST(schema.contains("R1"));
  T_clean_relation R1 = std::get<T_clean_relation>(schema["R1"]);
  BOOST_TEST(
      (R1.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R1.domains.size() == 1);
  BOOST_TEST(R1.domains[0] == "D1");

  BOOST_TEST(schema.contains("R2"));
  T_clean_relation R2 = std::get<T_clean_relation>(schema["R2"]);
  BOOST_TEST(
      (R2.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R2.domains.size() == 1);
  BOOST_TEST(R2.domains[0] == "D2");

  BOOST_TEST(schema.contains("R3"));
  T_clean_relation R3 = std::get<T_clean_relation>(schema["R3"]);
  BOOST_TEST(
      (R3.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R3.domains.size() == 2);
  BOOST_TEST(R3.domains[0] == "D1");
  BOOST_TEST(R3.domains[1] == "D2");

  BOOST_TEST(schema.contains("R4"));
  T_clean_relation R4 = std::get<T_clean_relation>(schema["R4"]);
  BOOST_TEST(
      (R4.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R4.domains.size() == 3);
  BOOST_TEST(R4.domains[0] == "D1");
  BOOST_TEST(R4.domains[1] == "D2");
  BOOST_TEST(R4.domains[2] == "D3");

  BOOST_TEST(schema.contains("R5"));
  T_clean_relation R5 = std::get<T_clean_relation>(schema["R5"]);
  BOOST_TEST(
      (R5.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R5.domains.size() == 4);
  BOOST_TEST(R5.domains[0] == "D1");
  BOOST_TEST(R5.domains[1] == "D2");
  BOOST_TEST(R5.domains[2] == "D3");
  BOOST_TEST(R5.domains[3] == "D1");
}

BOOST_AUTO_TEST_CASE(test_input_types) {
  T_schema schema = load_schema("test_schema/all_types.schema");

  BOOST_TEST(schema.contains("R1"));
  T_clean_relation R1 = std::get<T_clean_relation>(schema["R1"]);
  BOOST_TEST(
      (R1.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R1.domains.size() == 1);
  BOOST_TEST(R1.domains[0] == "D1");

  BOOST_TEST(schema.contains("R2"));
  T_clean_relation R2 = std::get<T_clean_relation>(schema["R2"]);
  BOOST_TEST((R2.distribution_spec.distribution == DistributionEnum::bigram));
  BOOST_TEST(R2.domains.size() == 1);
  BOOST_TEST(R2.domains[0] == "D1");

  BOOST_TEST(schema.contains("R3"));
  T_clean_relation R3 = std::get<T_clean_relation>(schema["R3"]);
  BOOST_TEST(
      (R3.distribution_spec.distribution == DistributionEnum::categorical));
  BOOST_TEST((R3.distribution_spec.distribution_args["k"] == "6"));
  BOOST_TEST(R3.domains.size() == 1);
  BOOST_TEST(R3.domains[0] == "D1");

  BOOST_TEST(schema.contains("R4"));
  T_clean_relation R4 = std::get<T_clean_relation>(schema["R4"]);
  BOOST_TEST((R4.distribution_spec.distribution == DistributionEnum::normal));
  BOOST_TEST(R4.domains.size() == 1);
  BOOST_TEST(R4.domains[0] == "D1");

  BOOST_TEST(schema.contains("R5"));
  T_clean_relation R5 = std::get<T_clean_relation>(schema["R5"]);
  BOOST_TEST((R5.distribution_spec.distribution == DistributionEnum::skellam));
  BOOST_TEST(R5.domains.size() == 1);
  BOOST_TEST(R5.domains[0] == "D1");

  BOOST_TEST(schema.contains("R6"));
  T_clean_relation R6 = std::get<T_clean_relation>(schema["R6"]);
  BOOST_TEST(
      (R6.distribution_spec.distribution == DistributionEnum::stringcat));
  BOOST_TEST((R6.distribution_spec.distribution_args["strings"] == "yes:no"));
  BOOST_TEST((R6.distribution_spec.distribution_args["delim"] == ":"));
  BOOST_TEST(R6.domains.size() == 1);
  BOOST_TEST(R6.domains[0] == "D1");
}

BOOST_AUTO_TEST_CASE(test_with_noisy) {
  T_schema schema = load_schema("test_schema/with_noisy.schema");

  BOOST_TEST(schema.contains("R1"));
  T_clean_relation R1 = std::get<T_clean_relation>(schema["R1"]);
  BOOST_TEST(
      (R1.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R1.domains.size() == 1);
  BOOST_TEST(R1.domains[0] == "D1");

  BOOST_TEST(schema.contains("R2"));
  T_noisy_relation R2 = std::get<T_noisy_relation>(schema["R2"]);
  BOOST_TEST((R2.emission_spec.emission == EmissionEnum::gaussian));
  BOOST_TEST(R2.domains.size() == 2);
  BOOST_TEST(R2.domains[0] == "D1");
  BOOST_TEST(R2.domains[1] == "D2");

  BOOST_TEST(schema.contains("R3"));
  T_clean_relation R3 = std::get<T_clean_relation>(schema["R3"]);
  BOOST_TEST(
      (R3.distribution_spec.distribution == DistributionEnum::bigram));
  BOOST_TEST(R3.domains.size() == 1);
  BOOST_TEST(R3.domains[0] == "D2");

  BOOST_TEST(schema.contains("R4"));
  T_noisy_relation R4 = std::get<T_noisy_relation>(schema["R4"]);
  BOOST_TEST((R4.emission_spec.emission == EmissionEnum::gaussian));
  BOOST_TEST(R4.domains.size() == 3);
  BOOST_TEST(R4.domains[0] == "D1");
  BOOST_TEST(R4.domains[1] == "D2");
  BOOST_TEST(R4.domains[2] == "D3");

  BOOST_TEST(schema.contains("R5"));
  T_noisy_relation R5 = std::get<T_noisy_relation>(schema["R5"]);
  BOOST_TEST((R5.emission_spec.emission == EmissionEnum::sometimes_bitflip));
  BOOST_TEST(R5.domains.size() == 1);
  BOOST_TEST(R5.domains[0] == "D1");

  BOOST_TEST(schema.contains("R6"));
  T_clean_relation R6 = std::get<T_clean_relation>(schema["R6"]);
  BOOST_TEST(
      (R6.distribution_spec.distribution == DistributionEnum::normal));
  BOOST_TEST(R6.domains.size() == 1);
  BOOST_TEST(R6.domains[0] == "D1");
}

BOOST_AUTO_TEST_CASE(test_incorporate_observations_irm) {
  T_clean_relation TR1({"D1", "D2"}, false, DistributionSpec("normal"));
  T_noisy_relation TR2({"D1", "D2", "D3"}, true, EmissionSpec("gaussian"),
                       "R1");
  T_schema schema = {{"R1", TR1}, {"R2", TR2}};

  T_observations observations = {{"R2",
                                  {
                                      {{"apple", "cat", "circle"}, 1.2},
                                      {{"banana", "dog", "square"}, 1.3},
                                      {{"apple", "dog", "triangle"}, 0.9},
                                      {{"apple", "cat", "square"}, 1.0},
                                      {{"banana", "cat", "rhombus"}, 1.1},
                                  }}};

  T_encoding encoding = encode_observations(schema, observations);
  IRM* irm = new IRM(schema);
  std::mt19937 prng;
  incorporate_observations(&prng, irm, encoding, observations);

  Relation<double>* R1 = std::get<Relation<double>*>(irm->get_relation("R1"));
  Relation<double>* R2 = std::get<Relation<double>*>(irm->get_relation("R2"));
  BOOST_TEST(R1->get_data().size() == 4);  // (apple, cat) is duplicated in R1.
  BOOST_TEST(R2->get_data().size() == 5);

  NoisyRelation<double>* NR2 = reinterpret_cast<NoisyRelation<double>*>(R2);
  BOOST_TEST(NR2->base_relation == R1);
}

BOOST_AUTO_TEST_CASE(test_incorporate_observations_relation_irm) {
  T_clean_relation TR1({"D1", "D2"}, false, DistributionSpec("normal"));
  T_noisy_relation TR2({"D1", "D2", "D3"}, true, EmissionSpec("gaussian"),
                       "R1");
  T_schema schema = {{"R1", TR1}, {"R2", TR2}};

  std::mt19937 prng;
  IRM* irm = new IRM(schema);
  T_encoded_observations encoded_observations = {
      {"R2", {{{1, 3, 5}, 2.3}, {{1, 3, 6}, 4.3}}}};
  std::unordered_map<std::string, std::string> noisy_to_base = {{"R2", "R1"}};
  std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
      relation_items = {{"R2", {{1, 3, 5}, {1, 3, 6}}}};
  std::unordered_set<std::string> completed_relations;
  incorporate_observations_relation(&prng, "R2", irm, encoded_observations,
                                    noisy_to_base, relation_items,
                                    completed_relations);
  // R1 was incorporated recursively.
  Relation<double>* R1 = std::get<Relation<double>*>(irm->get_relation("R1"));
  Relation<double>* R2 = std::get<Relation<double>*>(irm->get_relation("R2"));
  BOOST_TEST(R1->get_data().size() == 1);
  BOOST_TEST(R2->get_data().size() == 2);
}

BOOST_AUTO_TEST_CASE(test_incorporate_observations_hirm) {
  T_clean_relation TR1({"D1"}, false, DistributionSpec("normal"));
  T_noisy_relation TR2({"D1", "D2"}, false, EmissionSpec("gaussian"), "R1");
  T_noisy_relation TR3({"D1", "D2", "D3"}, true, EmissionSpec("gaussian"),
                       "R2");
  T_schema schema = {{"R2", TR2}, {"R3", TR3}, {"R1", TR1}};

  T_observations observations = {{"R3",
                                  {
                                      {{"apple", "cat", "circle"}, 1.2},
                                      {{"banana", "dog", "square"}, 1.3},
                                      {{"apple", "dog", "triangle"}, 0.9},
                                      {{"apple", "cat", "square"}, 1.0},
                                      {{"banana", "cat", "rhombus"}, 1.1},
                                  }}};

  T_encoding encoding = encode_observations(schema, observations);
  std::mt19937 prng;
  HIRM* hirm = new HIRM(schema, &prng);
  incorporate_observations(&prng, hirm, encoding, observations);

  Relation<double>* R1 = std::get<Relation<double>*>(hirm->get_relation("R1"));
  Relation<double>* R2 = std::get<Relation<double>*>(hirm->get_relation("R2"));
  Relation<double>* R3 = std::get<Relation<double>*>(hirm->get_relation("R3"));
  BOOST_TEST(R1->get_data().size() == 2);
  BOOST_TEST(R2->get_data().size() == 4);
  BOOST_TEST(R3->get_data().size() == 5);

  NoisyRelation<double>* NR2 = reinterpret_cast<NoisyRelation<double>*>(R2);
  NoisyRelation<double>* NR3 = reinterpret_cast<NoisyRelation<double>*>(R3);
  BOOST_TEST(NR2->base_relation == R1);
  BOOST_TEST(NR3->base_relation == R2);
}

BOOST_AUTO_TEST_CASE(test_incorporate_observations_relation_hirm) {
  T_clean_relation TR1({"D1"}, false, DistributionSpec("normal"));
  T_noisy_relation TR2({"D1", "D2"}, false, EmissionSpec("gaussian"), "R1");
  T_noisy_relation TR3({"D1", "D2", "D3"}, true, EmissionSpec("gaussian"),
                       "R2");
  T_schema schema = {{"R2", TR2}, {"R3", TR3}, {"R1", TR1}};

  std::mt19937 prng;
  HIRM* hirm = new HIRM(schema, &prng);
  T_encoded_observations encoded_observations = {
      {"R3", {{{1, 3, 5}, 2.3}, {{1, 3, 6}, 4.3}}}};
  std::unordered_map<std::string, std::string> noisy_to_base = {{"R3", "R2"},
                                                                {"R2", "R1"}};
  std::unordered_map<std::string, std::unordered_set<T_items, H_items>>
      relation_items = {{"R3", {{1, 3, 5}, {1, 3, 6}}}};
  std::unordered_set<std::string> completed_relations;
  incorporate_observations_relation(&prng, "R3", hirm, encoded_observations,
                                    noisy_to_base, relation_items,
                                    completed_relations);
  // R1 and R2 were incorporated recursively.
  Relation<double>* R1 = std::get<Relation<double>*>(hirm->get_relation("R1"));
  Relation<double>* R2 = std::get<Relation<double>*>(hirm->get_relation("R2"));
  Relation<double>* R3 = std::get<Relation<double>*>(hirm->get_relation("R3"));
  BOOST_TEST(R1->get_data().size() == 1);
  BOOST_TEST(R2->get_data().size() == 1);
  BOOST_TEST(R3->get_data().size() == 2);
}
