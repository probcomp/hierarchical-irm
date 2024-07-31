#define BOOST_TEST_MODULE test pclean_schema

#include "pclean/io.hh"
#include "pclean/schema_helper.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

struct SchemaTestFixture {
  SchemaTestFixture() {
    std::stringstream ss(R"""(
class School
  name ~ string
  degree_dist ~ categorical(k=100)

class Physician
  school ~ School
  degree ~ stringcat(strings="MD PT NP DO PHD")
  specialty ~ stringcat(strings="Family Med:Internal Med:Physical Therapy", delim=":")
  # observed_degree ~ maybe_swap(degree)

class City
  name ~ string
  state ~ stringcat(strings="AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY")

class Practice
  city ~ City

class Record
  physician ~ Physician
  location ~ Practice

observe
  physician.specialty as Specialty
  physician.school.name as School
  physician.degree as Degree
  location.city.name as City
  location.city.state as State
  from Record
)""");
    assert(read_schema(ss, &schema));
  }

  ~SchemaTestFixture() {}

  PCleanSchema schema;
};

BOOST_FIXTURE_TEST_SUITE(schema_test_suite, SchemaTestFixture)

BOOST_AUTO_TEST_CASE(test_get_class_by_name) {
  PCleanSchemaHelper schema_helper(schema);
  PCleanClass c = schema_helper.get_class_by_name("Practice");
  BOOST_TEST(c.name == "Practice");
}

BOOST_AUTO_TEST_CASE(test_domains_cache) {
  PCleanSchemaHelper schema_helper(schema);

  std::vector<std::string> expected_domains = {"School"};
  BOOST_TEST(schema_helper.domains["School"] == expected_domains);

  expected_domains = {"Physician", "school:School"};
  BOOST_TEST(schema_helper.domains["Physician"] == expected_domains);

  expected_domains = {"City"};
  BOOST_TEST(schema_helper.domains["City"] == expected_domains);

  expected_domains = {"Practice", "city:City"};
  BOOST_TEST(schema_helper.domains["Practice"] == expected_domains);

  expected_domains = {
    "Record", "physician:Physician", "physician:school:School",
    "location:Practice", "location:city:City"};
  BOOST_TEST(schema_helper.domains["Record"] == expected_domains);
}

BOOST_AUTO_TEST_CASE(test_domains_cache_two_paths_same_source) {
  std::stringstream ss(R"""(
class City
  name ~ string

class Person
  birth_city ~ City
  home_city ~ City
)""");
  PCleanSchema schema;
  assert(read_schema(ss, &schema));
  PCleanSchemaHelper schema_helper(schema);

  std::vector<std::string> expected_domains = {
    "Person", "birth_city:City", "home_city:City"};
  BOOST_TEST(schema_helper.domains["Person"] == expected_domains);
}

BOOST_AUTO_TEST_CASE(test_make_hirm_schmea) {
  PCleanSchemaHelper schema_helper(schema);
  T_schema tschema = schema_helper.make_hirm_schema();

  BOOST_TEST(tschema.contains("School:name"));
  T_clean_relation cr = std::get<T_clean_relation>(tschema["School:name"]);
  BOOST_TEST(!cr.is_observed);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::bigram));
  std::vector<std::string> expected_domains = {"School"};
  BOOST_TEST(cr.domains == expected_domains);

  BOOST_TEST(tschema.contains("School:degree_dist"));
  T_clean_relation cr2 = std::get<T_clean_relation>(tschema["School:degree_dist"]);
  BOOST_TEST((cr2.distribution_spec.distribution == DistributionEnum::categorical));
  BOOST_TEST(cr2.distribution_spec.distribution_args.contains("k"));
  BOOST_TEST(cr2.domains == expected_domains);

  BOOST_TEST(tschema.contains("Physician:degree"));
  T_clean_relation cr3 = std::get<T_clean_relation>(tschema["Physician:degree"]);
  BOOST_TEST((cr3.distribution_spec.distribution == DistributionEnum::stringcat));
  std::vector<std::string> expected_domains2 = {"Physician", "schoool:School"};
  BOOST_TEST(cr3.domains == expected_domains2);

  BOOST_TEST(tschema.contains("Physician:specialty"));

  BOOST_TEST(tschema.contains("City:name"));
  T_clean_relation cr4 = std::get<T_clean_relation>(tschema["City:name"]);
  std::vector<std::string> expected_domains3 = {"City"};
  BOOST_TEST(cr4.domains == expected_domains3);

  BOOST_TEST(tschema.contains("City:state"));

  BOOST_TEST(tschema.contains("Specialty"));
  T_noisy_relation nr1 = std::get<T_noisy_relation>(tschema["Specialty"]);
  BOOST_TEST(!nr1.is_observed);
  BOOST_TEST((nr1.emission_spec.emission == EmissionEnum::bigram_string));
  expected_domains = {
    "Record", "physician:Physician", "physician:school:School",
    "location:Practice", "location:city:City"};
  BOOST_TEST(nr1.domains == expected_domains);

  BOOST_TEST(tschema.contains("School"));
  T_noisy_relation nr2 = std::get<T_noisy_relation>(tschema["School"]);
  BOOST_TEST(!nr2.is_observed);
  BOOST_TEST((nr2.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr2.domains == expected_domains);

  BOOST_TEST(tschema.contains("Degree"));
  T_noisy_relation nr3 = std::get<T_noisy_relation>(tschema["Degree"]);
  BOOST_TEST(!nr3.is_observed);
  BOOST_TEST((nr3.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr3.domains == expected_domains);

  BOOST_TEST(tschema.contains("City"));
  T_noisy_relation nr4 = std::get<T_noisy_relation>(tschema["City"]);
  BOOST_TEST(!nr4.is_observed);
  BOOST_TEST((nr4.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr4.domains == expected_domains);

  BOOST_TEST(tschema.contains("State"));
  T_noisy_relation nr5 = std::get<T_noisy_relation>(tschema["State"]);
  BOOST_TEST(!nr5.is_observed);
  BOOST_TEST((nr5.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr5.domains == expected_domains);
}

BOOST_AUTO_TEST_SUITE_END()
