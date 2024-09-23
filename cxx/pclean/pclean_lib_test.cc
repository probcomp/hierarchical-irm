#define BOOST_TEST_MODULE test pclean_csv

#include "pclean/io.hh"
#include "pclean/pclean_lib.hh"
#include "pclean/schema_helper.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_translate_observations) {
  std::stringstream ss(R"""(Column1,Room Type,Monthly Rent,County,State
0,studio,,Mahoning County,OH
1,4br,2152.0,,NV
2,1br,1267.0,Gwinnett County,
)""");

  DataFrame df = DataFrame::from_csv(ss);

  std::map<std::string, std::string> state_params = {{"strings", "AL AK AZ AR CA CO CT DE DC FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY"}};
  std::map<std::string, std::string> br_params = {{"strings", "1br 2br 3br 4br studio"}};

  T_schema schema = {
    {"County:name",
      T_clean_relation{{"dCounty"}, false, DistributionSpec("bigram")}},
    {"County:state",
      T_clean_relation{{"dCounty"}, false, DistributionSpec("stringcat", state_params)}},
    {"Room Type",
      T_clean_relation{{"dObs"}, true, DistributionSpec("stringcat", br_params)}},
    {"Monthly Rent",
      T_clean_relation{{"dObs"}, true, DistributionSpec("normal")}},
    {"County",
      T_noisy_relation{{"dCounty", "dObs"}, true, EmissionSpec("bigram"), "County:name"}},
    {"State",
      T_noisy_relation{{"dCounty", "dObs"}, true, EmissionSpec("bigram"), "County:state"}}};

  std::map<std::string, std::vector<std::string>> annotated_domains_for_relations;
  annotated_domains_for_relations["Room Type"] = {"Obs"};
  annotated_domains_for_relations["Monthly Rent"] = {"Obs"};
  annotated_domains_for_relations["County"] = {"county:County", "Obs"};
  annotated_domains_for_relations["State"] = {"county:County", "Obs"};

  T_observations obs = translate_observations(
      df, schema, annotated_domains_for_relations);

  // Relations not corresponding to columns should be un-observed.
  BOOST_TEST(!obs.contains("County:name"));
  BOOST_TEST(!obs.contains("County:state"));

  BOOST_TEST(obs["Room Type"].size() == 3);
  BOOST_TEST(obs["Monthly Rent"].size() == 2);
  BOOST_TEST(obs["County"].size() == 2);
  BOOST_TEST(obs["State"].size() == 2);

  BOOST_TEST(std::get<0>(obs["Room Type"][0]).size() == 1);
  BOOST_TEST(std::get<1>(obs["Room Type"][0]) == "studio");

  BOOST_TEST(std::get<0>(obs["Monthly Rent"][0]).size() == 1);
  BOOST_TEST(std::get<1>(obs["Monthly Rent"][0]) == "2152.0");

  BOOST_TEST(std::get<0>(obs["County"][0]).size() == 2);
  BOOST_TEST(std::get<1>(obs["County"][0]) == "Mahoning County");

  BOOST_TEST(std::get<0>(obs["State"][0]).size() == 2);
  BOOST_TEST(std::get<1>(obs["State"][0]) == "OH");
}

BOOST_AUTO_TEST_CASE(test_make_pclean_samples) {
  std::mt19937 prng;
  std::map<std::string, std::vector<std::string>> annotated_domains_for_relation;

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

  PCleanSchemaHelper schema_helper(pclean_schema);
  T_schema hirm_schema = schema_helper.make_hirm_schema(
      &annotated_domains_for_relation);

  HIRM hirm(hirm_schema, &prng);

  // TODO: Re-enable test when it's fixed to sample non-duplicate entities.
  // printf("DEBUG: before\n");
  // DataFrame samples = make_pclean_samples(
  //     10, &hirm, pclean_schema, annotated_domains_for_relation, &prng);
  // printf("DEBUG: after\n");
  // BOOST_TEST(samples.data["Specialty"].size() == 10);
  // BOOST_TEST(samples.data["School"].size() == 10);
  // BOOST_TEST(samples.data["Degree"].size() == 10);
  // BOOST_TEST(samples.data["City"].size() == 10);
  // BOOST_TEST(samples.data["State"].size() == 10);
}
