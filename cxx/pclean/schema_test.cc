#define BOOST_TEST_MODULE test pclean_schema

#include "pclean/io.hh"
#include "pclean/schema.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

struct SchemaTestFixture {
  SchemaTestFixture() {
    std::stringstream ss(R"""(
class School
  name ~ bigram
  degree_dist ~ categorical[num_classes=100]

class Physician
  school ~ School
  degree ~ stringcat[strings="MD PT NP DO PHD"]
  specialty ~ stringcat[strings="Family Med:Internal Med:Physical Therapy", delim=":"]
  # observed_degree ~ maybe_swap(degree)

class City
  name ~ bigram
  state ~ stringcat[strings="AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY"]

class Practice
  city ~ City
  bad_city ~ bigram(city.name)

class Record
  physician ~ Physician
  location ~ Practice

observe
  physician.specialty as Specialty
  physician.school.name as School
  physician.observed_degree as Degree
  location.bad_city as City
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

BOOST_AUTO_TEST_CASE(test_get_parent_classes) {
  PCleanSchemaHelper schema_helper(schema);
  BOOST_TEST(schema_helper.get_parent_classes("School").empty());
  BOOST_TEST(schema_helper.get_parent_classes("City").empty());
  BOOST_TEST(schema_helper.get_parent_classes("Physician").size() == 1);
  BOOST_TEST(schema_helper.get_parent_classes("Practice").size() == 1);
  BOOST_TEST(schema_helper.get_parent_classes("Record").size() == 2);
}

BOOST_AUTO_TEST_CASE(test_get_ancestor_classes) {
  PCleanSchemaHelper schema_helper(schema);
  BOOST_TEST(schema_helper.get_ancestor_classes("School").empty());
  BOOST_TEST(schema_helper.get_ancestor_classes("City").empty());
  BOOST_TEST(schema_helper.get_ancestor_classes("Physician").size() == 1);
  BOOST_TEST(schema_helper.get_ancestor_classes("Practice").size() == 1);
  BOOST_TEST(schema_helper.get_ancestor_classes("Record").size() == 4);
}

BOOST_AUTO_TEST_CASE(test_get_source_classes) {
  PCleanSchemaHelper schema_helper(schema);
  BOOST_TEST(schema_helper.get_source_classes("School").empty());
  BOOST_TEST(schema_helper.get_source_classes("City").empty());
  BOOST_TEST(schema_helper.get_source_classes("Physician").size() == 1);
  BOOST_TEST(schema_helper.get_source_classes("Practice").size() == 1);
  BOOST_TEST(schema_helper.get_source_classes("Record").size() == 2);
}


BOOST_AUTO_TEST_CASE(test_make_hirm_schmea) {
  PCleanSchemaHelper schema_helper(schema);
  T_schema tschema = schema_helper.make_hirm_schema();
  BOOST_TEST(tschema.count("School:name") == 1);
  BOOST_TEST(tschema.count("School:degree_dist") == 1);
  BOOST_TEST(tschema.count("Physician:degree") == 1);
  BOOST_TEST(tschema.count("Physician:specialty") == 1);
  BOOST_TEST(tschema.count("City:name") == 1);
  BOOST_TEST(tschema.count("City:state") == 1);
  BOOST_TEST(tschema.count("Practice:bad_city") == 1);
}

BOOST_AUTO_TEST_SUITE_END()
