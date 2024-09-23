#define BOOST_TEST_MODULE test pclean_schema

#include "pclean/schema_helper.hh"

#include <boost/test/included/unit_test.hpp>
#include <sstream>

#include "pclean/io.hh"
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
    [[maybe_unused]] bool ok = read_schema(ss, &schema);
    assert(ok);
  }

  ~SchemaTestFixture() {}

  PCleanSchema schema;
};

BOOST_FIXTURE_TEST_SUITE(schema_test_suite, SchemaTestFixture)

BOOST_AUTO_TEST_CASE(test_domains_cache) {
  PCleanSchemaHelper schema_helper(schema);

  std::vector<std::string> expected_domains = {"School"};
  std::vector<std::string> expected_annotated_domains = {"School"};
  BOOST_TEST(schema_helper.domains["School"] == expected_domains);
  BOOST_TEST(schema_helper.annotated_domains["School"] ==
             expected_annotated_domains);

  expected_domains = {"School", "Physician"};
  expected_annotated_domains = {"school:School", "Physician"};
  BOOST_TEST(schema_helper.domains["Physician"] == expected_domains);
  BOOST_TEST(schema_helper.annotated_domains["Physician"] ==
             expected_annotated_domains);

  expected_domains = {"City"};
  expected_annotated_domains = {"City"};
  BOOST_TEST(schema_helper.domains["City"] == expected_domains);
  BOOST_TEST(schema_helper.annotated_domains["City"] ==
             expected_annotated_domains);

  expected_domains = {"City", "Practice"};
  expected_annotated_domains = {"city:City", "Practice"};
  BOOST_TEST(schema_helper.domains["Practice"] == expected_domains);
  BOOST_TEST(schema_helper.annotated_domains["Practice"] ==
             expected_annotated_domains);

  expected_domains = {"City", "Practice", "School", "Physician", "Record"};
  expected_annotated_domains = {"location:city:City", "location:Practice",
                                "physician:school:School",
                                "physician:Physician", "Record"};
  BOOST_TEST(schema_helper.domains["Record"] == expected_domains,
             tt::per_element());
  BOOST_TEST(
      schema_helper.annotated_domains["Record"] == expected_annotated_domains,
      tt::per_element());

  auto& ref_indices = schema_helper.class_reference_indices;

  // The Practice, Physician, and Record classes have reference fields, so they
  // should be included in the reference field index map.
  BOOST_TEST(ref_indices.size() == 3);

  // For Physician and Practice, index 1 corresponds to the class itself, and
  // index 0 corresponds to the reference class.
  BOOST_TEST(ref_indices.at("Physician").at(1).at("school") == 0);
  BOOST_TEST(ref_indices.at("Practice").at(1).at("city") == 0);

  // For Record, index 4 corresponds to the class itself, which points to
  // physician (index 3) and location (index 1).
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
  PCleanSchemaHelper schema_helper(schema);

  std::vector<std::string> expected_domains = {"City", "City", "Person"};
  std::vector<std::string> expected_annotated_domains = {
      "birth_city:City", "home_city:City", "Person"};
  BOOST_TEST(schema_helper.domains["Person"] == expected_domains,
             tt::per_element());
  BOOST_TEST(
      schema_helper.annotated_domains["Person"] == expected_annotated_domains,
      tt::per_element());

  auto& ref_indices = schema_helper.class_reference_indices;

  // Only the Person field has reference fields.
  BOOST_TEST(ref_indices.size() == 1);
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
  PCleanSchemaHelper schema_helper(schema);

  std::vector<std::string> expected_domains = {"City", "Practice", "City",
                                               "School", "Physician"};
  std::vector<std::string> expected_annotated_domains = {
      "practice:location:City", "practice:Practice", "school:location:City",
      "school:School", "Physician"};
  BOOST_TEST(schema_helper.domains["Physician"] == expected_domains,
             tt::per_element());
  BOOST_TEST(schema_helper.annotated_domains["Physician"] ==
                 expected_annotated_domains,
             tt::per_element());

  auto& ref_indices = schema_helper.class_reference_indices;

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
  PCleanSchemaHelper schema_helper(schema);
  T_schema tschema;

  PCleanClass query_class = schema.classes[schema.query.record_class];
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  schema_helper.make_relations_for_queryfield(schema.query.fields["School"],
                                              query_class, &tschema,
                                              &annotated_domains_for_relation);

  BOOST_TEST(tschema.size() == 2);
  BOOST_TEST(tschema.contains("School"));
  BOOST_TEST(tschema.contains("Physician::School"));
  BOOST_TEST(std::get<T_noisy_relation>(tschema["School"]).is_observed);
  BOOST_TEST(
      !std::get<T_noisy_relation>(tschema["Physician::School"]).is_observed);

  std::vector<std::string> expected_adfr = {"physician:school:School",
                                            "physician:Physician", "Record"};
  BOOST_TEST(annotated_domains_for_relation["School"] == expected_adfr,
             tt::per_element());
}

BOOST_AUTO_TEST_CASE(test_make_relations_for_queryfield_only_final_emissions) {
  PCleanSchemaHelper schema_helper(schema, true);
  T_schema tschema;

  PCleanClass query_class = schema.classes[schema.query.record_class];
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  schema_helper.make_relations_for_queryfield(schema.query.fields["School"],
                                              query_class, &tschema,
                                              &annotated_domains_for_relation);

  BOOST_TEST(tschema.size() == 1);
  BOOST_TEST(tschema.contains("School"));
}

BOOST_AUTO_TEST_CASE(test_make_hirm_schmea) {
  PCleanSchemaHelper schema_helper(schema);
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  T_schema tschema =
      schema_helper.make_hirm_schema(&annotated_domains_for_relation);

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

  auto& ref_indices = schema_helper.relation_reference_indices;

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
  PCleanSchemaHelper schema_helper(schema, true);
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  T_schema tschema =
      schema_helper.make_hirm_schema(&annotated_domains_for_relation);

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

  auto& ref_indices = schema_helper.relation_reference_indices;
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

  PCleanSchemaHelper schema_helper(schema2, false, true);
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  T_schema tschema =
      schema_helper.make_hirm_schema(&annotated_domains_for_relation);

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

  PCleanSchemaHelper schema_helper(schema2, false, false);
  std::map<std::string, std::vector<std::string>>
      annotated_domains_for_relation;
  T_schema tschema =
      schema_helper.make_hirm_schema(&annotated_domains_for_relation);

  BOOST_TEST(tschema.contains("Record:rent"));
  BOOST_TEST(tschema.contains("Rent"));

  T_clean_relation cr = std::get<T_clean_relation>(tschema["Record:rent"]);
  BOOST_TEST(!cr.is_observed);
  T_noisy_relation nr = std::get<T_noisy_relation>(tschema["Rent"]);
  BOOST_TEST(nr.is_observed);

  std::vector<std::string> expected_adfr = {"Record"};
  BOOST_TEST(annotated_domains_for_relation["Rent"] == expected_adfr);
}

BOOST_AUTO_TEST_SUITE_END()
