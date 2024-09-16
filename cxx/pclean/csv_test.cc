#define BOOST_TEST_MODULE test pclean_csv

#include "pclean/csv.hh"
#include <sstream>
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_read_csv_no_header) {
  std::stringstream ss(R"""(
1,2,3,4
monsters,walking,across,the floor
i,love,counting,counting to the number four
oh you're counting,counting with me,to one less than five,and one more than three
oh,we're,counting,to four
oh,let's,count,some more
1,2,3,4
Penguins,that,went,by the door
1,2,3,4
Chickens,just,back,from the shore
)""");
  DataFrame df = DataFrame::from_csv(ss, {"A", "B", "C", "D"});
  BOOST_TEST(df.data["A"].size() == 10);
  BOOST_TEST(df.data["B"][0] == "2");
  BOOST_TEST(df.data["C"][1] == "across");
  BOOST_TEST(df.data["D"][2] == "counting to the number four");
}

BOOST_AUTO_TEST_CASE(test_read_csv_has_header) {
  std::stringstream ss(R"""(
Name,Specialty,Degree,School,Address,City,State,Zip
K. Ryan,Family Medicine,DO,PCOM,6317 York Rd,Baltimore,MD,21212
K. Ryan,Family Medicine,DO,PCOM,100 Walter Ward Blvd,Abingdon,MD,21009
S. Evans,Internal Medicine,MD,UMD,100 Walter Ward Blvd,Abingdon,MD,21009
M. Grady,Physical Therapy,PT,Other,3491 Merchants Blvd,Abingdon,MD,21009
)""");
  DataFrame df = DataFrame::from_csv(ss);
  BOOST_TEST(df.data["Name"].size() == 4);
  BOOST_TEST(df.data["Specialty"][0] == "Family Medicine");
  BOOST_TEST(df.data["Degree"][1] == "DO");
  BOOST_TEST(df.data["School"][2] == "UMD");
  BOOST_TEST(df.data["Address"][3] == "3491 Merchants Blvd");
}

BOOST_AUTO_TEST_CASE(test_carriage_ret_at_end_of_line) {
  std::stringstream ss("A,B,C\r\na,b,c\r\n");
  DataFrame df = DataFrame::from_csv(ss);
  BOOST_TEST(df.data["C"][0] == "c");
}

BOOST_AUTO_TEST_CASE(test_round_trip) {
  std::string s = R"""(
Name,Specialty,Degree,School,Address,City,State,Zip
K. Ryan,Family Medicine,DO,PCOM,6317 York Rd,Baltimore,MD,21212
K. Ryan,Family Medicine,DO,PCOM,100 Walter Ward Blvd,Abingdon,MD,21009
S. Evans,Internal Medicine,MD,UMD,100 Walter Ward Blvd,Abingdon,MD,21009
M. Grady,Physical Therapy,PT,Other,3491 Merchants Blvd,Abingdon,MD,21009
)""";
  std::stringstream ss(s);
  DataFrame df = DataFrame::from_csv(ss);
  std::stringstream oss;
  BOOST_TEST(df.to_csv(oss));
  // Order isn't preserved because map sorts keys alphabetically.
  std::string s2 = R"""(Address,City,Degree,Name,School,Specialty,State,Zip
6317 York Rd,Baltimore,DO,K. Ryan,PCOM,Family Medicine,MD,21212
100 Walter Ward Blvd,Abingdon,DO,K. Ryan,PCOM,Family Medicine,MD,21009
100 Walter Ward Blvd,Abingdon,MD,S. Evans,UMD,Internal Medicine,MD,21009
3491 Merchants Blvd,Abingdon,PT,M. Grady,Other,Physical Therapy,MD,21009
)""";
  BOOST_TEST(s2 == oss.str(), tt::per_element());
}
