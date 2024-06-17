// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test SimpleString

#include "emissions/simple_string.hh"

#include <boost/test/included/unit_test.hpp>
#include <random>

BOOST_AUTO_TEST_CASE(test_simple) {
  SimpleStringEmission ss;

  double orig_lp = ss.logp_score();
  BOOST_TEST(ss.N == 0);
  ss.incorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  BOOST_TEST(ss.N == 1);
  BOOST_TEST(ss.substitutions.N == 5);
  BOOST_TEST(ss.substitutions.s == 0);
  BOOST_TEST(ss.deletions.N == 5);
  BOOST_TEST(ss.deletions.s == 0);
  BOOST_TEST(ss.insertions.N == 6);
  BOOST_TEST(ss.insertions.s == 0);
  ss.unincorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  BOOST_TEST(ss.N == 0);
  BOOST_TEST(ss.substitutions.N == 0);
  BOOST_TEST(ss.deletions.N == 0);
  BOOST_TEST(ss.insertions.N == 0);
  BOOST_TEST(orig_lp == ss.logp_score());

  ss.incorporate(std::make_pair<std::string, std::string>("world", "w0rld!"));
  BOOST_TEST(ss.substitutions.N == 5);
  // If our alignment algorithm was smart, it would see only one subsitution.
  // But it is dumb, so it sees four of them.
  BOOST_TEST(ss.substitutions.s == 4);
  BOOST_TEST(ss.deletions.N == 5);
  BOOST_TEST(ss.deletions.s == 0);
  // There are six insertion loops:  one before the "w", one between "w" and
  // "o", ..., and one after "d".  But because there is an actual insertion
  // in this example, there are seven "opportunities for insertion" because
  // we could have had additional insertions at the same place as the actual
  // insertion.
  BOOST_TEST(ss.insertions.N == 7);
  BOOST_TEST(ss.insertions.s == 1);
  ss.incorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  BOOST_TEST(ss.N == 2);

  BOOST_TEST(ss.logp(std::make_pair<std::string, std::string>("test", "tst")) <
             0.0);
  BOOST_TEST(ss.N == 2);
  // Since we have incorporated one substitution and no deletions, we should
  // expect substitutions to be more likely.
  double lp1 = ss.logp(std::make_pair<std::string, std::string>("test", "tst"));
  double lp2 =
      ss.logp(std::make_pair<std::string, std::string>("test", "te$t"));
  BOOST_TEST(lp1 < lp2);

  std::mt19937 prng;
  std::string corrupted = ss.sample_corrupted("clean", &prng);
  BOOST_TEST(corrupted.length() > 3);
  BOOST_TEST(corrupted.length() < 7);

  BOOST_TEST(ss.propose_clean({"clean", "clean!", "cl5an", "lean"}, &prng) ==
             "clean");
}
