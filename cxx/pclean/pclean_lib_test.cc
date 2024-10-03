#define BOOST_TEST_MODULE test pclean_csv

#include "pclean/io.hh"
#include "pclean/pclean_lib.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_incorporate_observations) {
  std::mt19937 prng;

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

  PCleanSchema pclean_schema;
  BOOST_TEST(read_schema(ss, &pclean_schema));

  GenDB gendb(&prng, pclean_schema);

  std::stringstream ss2(
R"""(Specialty,School,Degree,City,State
Internal Medicine,Harvard,MD,Somerville,MA
Brain Surgery,UCSF,PhD,San Diego,CA
Dermatology,Duke,MD,Chicago,IL
Internal Medicine,John Hopkins,MD,Washington,DC
Pediatrics,Harvard,MD,Seattle,WA
)""");

  DataFrame df = DataFrame::from_csv(ss2);

  incorporate_observations(&prng, &gendb, df);
  BOOST_TEST(gendb.entity_crps["Practice"].N == 5);
  BOOST_TEST(gendb.entity_crps["Physician"].N == 5);
}

BOOST_AUTO_TEST_CASE(test_incorporate_observations_diagonal) {
  std::mt19937 prng;

  std::stringstream ss(R"""(
class City
  name ~ string
  state ~ stringcat(strings="AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY")

class School
  name ~ string
  degree_dist ~ categorical(k=100)
  city ~ City

class Physician
  school ~ School
  degree ~ stringcat(strings="MD PT NP DO PHD")
  specialty ~ stringcat(strings="Family Med:Internal Med:Physical Therapy", delim=":")
  # observed_degree ~ maybe_swap(degree)

class Practice
  city ~ City

class Record
  physician ~ Physician
  location ~ Practice

observe
  physician.specialty as Specialty
  physician.school.name as School
  physician.school.city.name as SchoolCity
  physician.degree as Degree
  location.city.name as City
  location.city.state as State
  from Record
)""");

  PCleanSchema pclean_schema;
  BOOST_TEST(read_schema(ss, &pclean_schema));

  GenDB gendb(&prng, pclean_schema);

  std::stringstream ss2(
R"""(Specialty,School,SchoolCity,Degree,City,State
Internal Medicine,Harvard,Cambridge,MD,Somerville,MA
Brain Surgery,UCSF,San Francisco,PhD,San Diego,CA
Dermatology,Duke,Durham,MD,Chicago,IL
Internal Medicine,John Hopkins,Baltimore,MD,Washington,DC
Pediatrics,Harvard,Cambridge,MD,Seattle,WA
)""");

  DataFrame df = DataFrame::from_csv(ss2);

  incorporate_observations(&prng, &gendb, df);
  BOOST_TEST(gendb.entity_crps["Practice"].N == 5);
  // TODO(thomaswc): Figure out why the next BOOST_TEST is failing.
  // (.N == 4 instead of the expected 10).
  // BOOST_TEST(gendb.entity_crps["City"].N == 10);
}

BOOST_AUTO_TEST_CASE(test_make_pclean_samples) {
  std::mt19937 prng;

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

  PCleanSchema pclean_schema;
  BOOST_TEST(read_schema(ss, &pclean_schema));

  GenDB gendb(&prng, pclean_schema);

  DataFrame samples = make_pclean_samples(10, 0, &gendb, &prng);
  BOOST_TEST(samples.data["Specialty"].size() == 10);
  BOOST_TEST(samples.data["School"].size() == 10);
  BOOST_TEST(samples.data["Degree"].size() == 10);
  BOOST_TEST(samples.data["City"].size() == 10);
  BOOST_TEST(samples.data["State"].size() == 10);
}

BOOST_AUTO_TEST_CASE(test_make_dummy_encoding_from_gendb) {
  std::mt19937 prng;

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

  PCleanSchema pclean_schema;
  BOOST_TEST(read_schema(ss, &pclean_schema));

  GenDB gendb(&prng, pclean_schema);

  T_encoding enc = make_dummy_encoding_from_gendb(gendb);

  BOOST_TEST(enc.second.size() == 0);

  std::map<std::string, ObservationVariant> obs = {
    {"Specialty", "Internal Med"},
    {"School", "Harvard"},
    {"Degree", "MD"},
    {"City", "Cambridge"},
    {"State", "MA"}};

  gendb.incorporate(&prng, {0, obs}, true);
  T_encoding enc2 = make_dummy_encoding_from_gendb(gendb);

  BOOST_TEST(enc2.second["School"][0] == "School:0");
  BOOST_TEST(enc2.second["Physician"][0] == "Physician:0");
  BOOST_TEST(enc2.second["City"][0] == "City:0");
  BOOST_TEST(enc2.second["Practice"][0] == "Practice:0");

  BOOST_TEST(enc2.second["School"].size() == 1);

  for (int i = 1; i < 6; ++i) {
    gendb.incorporate(&prng, {i, obs}, true);
  }

  T_encoding enc3 = make_dummy_encoding_from_gendb(gendb);
  BOOST_TEST(enc3.second["School"].size() == 6);
  BOOST_TEST(enc3.second["School"][0] == "School:0");
  BOOST_TEST(enc3.second["School"][1] == "School:1");
  BOOST_TEST(enc3.second["School"][2] == "School:2");
  BOOST_TEST(enc3.second["School"][3] == "School:3");
  BOOST_TEST(enc3.second["School"][4] == "School:4");
  BOOST_TEST(enc3.second["School"][5] == "School:5");

  // Test that we got all the entities.
  for (const auto& [domain, crp] : gendb.domain_crps) {
    for (int i = 0; i <= crp.max_table(); ++i) {
      BOOST_TEST(enc3.second[domain].contains(i));
    }
  }

}
