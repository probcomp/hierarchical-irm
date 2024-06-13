// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test SimpleString

#include <random>

#include "emissions/simple_string.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_simple) {
  SimpleStringEmission ss;

  double orig_lp = ss.logp_score();
  BOOST_TEST(ss.N == 0);
  ss.incorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  BOOST_TEST(ss.N == 1);
  ss.unincorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  BOOST_TEST(ss.N == 0);
  BOOST_TEST(orig_lp == ss.logp_score());

  ss.incorporate(std::make_pair<std::string, std::string>("hello", "hello"));
  ss.incorporate(std::make_pair<std::string, std::string>("world", "w0rld!"));
  BOOST_TEST(ss.N == 2);

  BOOST_TEST(ss.logp(std::make_pair<std::string, std::string>("test", "tst")) < 0.0);

  std::mt19937 prng;
  std::string corrupted = ss.sample_corrupted("clean", &prng);
  BOOST_TEST(corrupted.length() > 3);
  BOOST_TEST(corrupted.length() < 7);

  BOOST_TEST(ss.propose_clean({"clean", "clean!", "cl5an", "lean"}, &prng)
             == "clean");
}
