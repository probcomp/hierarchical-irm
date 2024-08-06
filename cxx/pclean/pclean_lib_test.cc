#define BOOST_TEST_MODULE test pclean_csv

#include "pclean/pclean_lib.hh"
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
      T_clean_relation{{"dObs"}, false, DistributionSpec("stringcat", br_params)}},
    {"Monthly Rent",
      T_clean_relation{{"dObs"}, false, DistributionSpec("normal")}},
    {"County",
      T_noisy_relation{{"dCounty", "dObs"}, false, EmissionSpec("bigram"), "County:name"}},
    {"State",
      T_noisy_relation{{"dCounty", "dObs"}, false, EmissionSpec("bigram"), "County:state"}}};

  T_observations obs = translate_observations(df, schema);

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

