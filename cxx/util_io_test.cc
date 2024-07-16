#define BOOST_TEST_MODULE test UtilIO

#include "util_io.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_nary_relation) {
  T_schema schema = load_schema("test_schema/nary.schema");
  // Check that the relations are correct.
  BOOST_TEST(schema.contains("R1"));
  T_clean_relation R1 = std::get<T_clean_relation>(schema["R1"]);
  BOOST_TEST((R1.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R1.domains.size() == 1);
  BOOST_TEST(R1.domains[0] == "D1");

  BOOST_TEST(schema.contains("R2"));
  T_clean_relation R2 = std::get<T_clean_relation>(schema["R2"]);
  BOOST_TEST((R2.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R2.domains.size() == 1);
  BOOST_TEST(R2.domains[0] == "D2");

  BOOST_TEST(schema.contains("R3"));
  T_clean_relation R3 = std::get<T_clean_relation>(schema["R3"]);
  BOOST_TEST((R3.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R3.domains.size() == 2);
  BOOST_TEST(R3.domains[0] == "D1");
  BOOST_TEST(R3.domains[1] == "D2");

  BOOST_TEST(schema.contains("R4"));
  T_clean_relation R4 = std::get<T_clean_relation>(schema["R4"]);
  BOOST_TEST((R4.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R4.domains.size() == 3);
  BOOST_TEST(R4.domains[0] == "D1");
  BOOST_TEST(R4.domains[1] == "D2");
  BOOST_TEST(R4.domains[2] == "D3");

  BOOST_TEST(schema.contains("R5"));
  T_clean_relation R5 = std::get<T_clean_relation>(schema["R5"]);
  BOOST_TEST((R5.distribution_spec.distribution == DistributionEnum::bernoulli));
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
  BOOST_TEST((R1.distribution_spec.distribution == DistributionEnum::bernoulli));
  BOOST_TEST(R1.domains.size() == 1);
  BOOST_TEST(R1.domains[0] == "D1");

  BOOST_TEST(schema.contains("R2"));
  T_clean_relation R2 = std::get<T_clean_relation>(schema["R2"]);
  BOOST_TEST((R2.distribution_spec.distribution == DistributionEnum::bigram));
  BOOST_TEST(R2.domains.size() == 1);
  BOOST_TEST(R2.domains[0] == "D1");

  BOOST_TEST(schema.contains("R3"));
  T_clean_relation R3 = std::get<T_clean_relation>(schema["R3"]);
  BOOST_TEST((R3.distribution_spec.distribution == DistributionEnum::categorical));
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
  BOOST_TEST((R6.distribution_spec.distribution == DistributionEnum::stringcat));
  BOOST_TEST((R6.distribution_spec.distribution_args["strings"] == "yes:no"));
  BOOST_TEST((R6.distribution_spec.distribution_args["delim"] == ":"));
  BOOST_TEST(R6.domains.size() == 1);
  BOOST_TEST(R6.domains[0] == "D1");
}
