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
  degree_dist ~ categorical[num_classes=100]

class Physician
  school ~ School
  degree ~ stringcat[strings="MD PT NP DO PHD"]
  specialty ~ stringcat[strings="Family Med:Internal Med:Physical Therapy", delim=":"]
  observed_degree ~ maybe_swap(degree)  # TODO: handle p_err ~ beta(1, 1000)

class City
  name ~ bigram
  # Gary Gulman has a good routine about how the states got their two letter
  # abbreviations.
  state ~ strigcat[strings="AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY"]

class Practice # makes perfect!
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
  PCleanSchema schema;
  BOOST_TEST(read_schema(ss, &schema));
  BOOST_TEST(schema.query.base_class == "Record");

  BOOST_TEST(schema.query.fields.size() == 5);
  BOOST_TEST(schema.query.fields[0].name == "Specialty");
  std::vector<std::string> expected_path = {"physician", "specialty"};
  BOOST_TEST(schema.query.fields[0].class_path == expected_path,
             tt::per_element());
  BOOST_TEST(schema.query.fields[1].name == "School");
  std::vector<std::string> expected_path2 = {"physician", "school", "name"};
  BOOST_TEST(schema.query.fields[1].class_path == expected_path2,
             tt::per_element());

  BOOST_TEST(schema.classes.size() == 5);
  BOOST_TEST(schema.classes[0].name == "School");
  BOOST_TEST(schema.classes[0].vars.size() == 2);
  BOOST_TEST(schema.classes[0].vars[0].name == "name");
  BOOST_TEST(std::get<DistributionVar>(schema.classes[0].vars[0].spec).distribution_name == "bigram");
  BOOST_TEST(schema.classes[0].vars[1].name == "degree_dist");
  BOOST_TEST(std::get<DistributionVar>(schema.classes[0].vars[1].spec).distribution_name == "categorical");
  BOOST_TEST(std::get<DistributionVar>(schema.classes[0].vars[1].spec).distribution_params["num_classes"] == "100");

  BOOST_TEST(schema.classes[1].name == "Physician");
  BOOST_TEST(schema.classes[1].vars.size() == 4);
  BOOST_TEST(schema.classes[1].vars[0].name == "school");
  BOOST_TEST(std::get<ClassVar>(schema.classes[1].vars[0].spec).class_name == "School");
  BOOST_TEST(schema.classes[1].vars[3].name == "observed_degree");
  BOOST_TEST(std::get<EmissionVar>(schema.classes[1].vars[3].spec).emission_name == "maybe_swap");
  std::vector<std::string> expected_path3 = {"degree"};
  BOOST_TEST(std::get<EmissionVar>(schema.classes[1].vars[3].spec).field_path
             == expected_path3, tt::per_element());
}
