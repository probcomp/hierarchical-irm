#define BOOST_TEST_MODULE test pclean_io

#include "pclean/io.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_read_schema) {
  std::stringstream ss(R"""(
# Hello!  This is my test schema string.  I hope you like it.
class School
  name ~ bigram  # TODO: make this prefer school_names
  degree_dist ~ categorical(num_classes=100)

class Physician
  school ~ School
  degree ~ stringcat(strings="MD PT NP DO PHD")
  specialty ~ stringcat(strings="Family Med:Internal Med:Physical Therapy", delim=":")
  # observed_degree ~ maybe_swap(degree)  # TODO: handle p_err ~ beta(1, 1000)

class City
  name ~ bigram
  # Gary Gulman has a good routine about how the states got their two letter
  # abbreviations.
  state ~ strigcat(strings="AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY")

class Practice # makes perfect!
  city ~ City

class Record
  physician ~ Physician
  location ~ Practice

observe
  physician.specialty as Specialty
  physician.school.name as School
  physician.observed_degree as Degree
  location.city.name as City
  location.city.state as State
  from Record
)""");
  PCleanSchema schema;
  BOOST_TEST(read_schema(ss, &schema));
  BOOST_TEST(schema.query.record_class == "Record");

  BOOST_TEST(schema.query.fields.size() == 5);
  BOOST_TEST(schema.query.fields["Specialty"].name == "Specialty");
  std::vector<std::string> expected_path = {"physician", "specialty"};
  BOOST_TEST(schema.query.fields["Specialty"].class_path == expected_path,
             tt::per_element());
  BOOST_TEST(schema.query.fields["School"].name == "School");
  std::vector<std::string> expected_path2 = {"physician", "school", "name"};
  BOOST_TEST(schema.query.fields["School"].class_path == expected_path2,
             tt::per_element());

  BOOST_TEST(schema.classes.size() == 5);
  BOOST_TEST(schema.classes["School"].vars.size() == 2);
  BOOST_TEST(schema.classes["School"].vars.contains("name"));
  BOOST_TEST(std::get<ScalarVar>(schema.classes["School"].vars["name"].spec).joint_name == "bigram");
  BOOST_TEST(schema.classes["School"].vars.contains("degree_dist"));
  BOOST_TEST(std::get<ScalarVar>(schema.classes["School"].vars["degree_dist"].spec).joint_name == "categorical");
  BOOST_TEST(std::get<ScalarVar>(schema.classes["School"].vars["degree_dist"].spec).params["num_classes"] == "100");

  BOOST_TEST(schema.classes.contains("Physician"));
  BOOST_TEST(schema.classes["Physician"].vars.size() == 3);
  BOOST_TEST(schema.classes["Physician"].vars.contains("school"));
  BOOST_TEST(std::get<ClassVar>(schema.classes["Physician"].vars["school"].spec).class_name == "School");
}
