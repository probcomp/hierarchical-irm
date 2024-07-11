// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test util_parse

#include <map>
#include <string>

#include "util_parse.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_noargs) {
  std::string name;
  std::map<std::string, std::string> args;
  parse_name_and_parameters("hello", &name, &args);
  BOOST_TEST(name == "hello");

  BOOST_TEST(args.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_noargs2) {
  std::string name;
  std::map<std::string, std::string> args;
  parse_name_and_parameters("world()", &name, &args);
  BOOST_TEST(name == "world");

  BOOST_TEST(args.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_args) {
  std::string name;
  std::map<std::string, std::string> args;
  parse_name_and_parameters("person(name=bob, age=50)", &name, &args);
  BOOST_TEST(name == "person");
  BOOST_TEST(args.size() == 2);
  BOOST_TEST(args["name"] == "bob");
  BOOST_TEST(args["age"] == "50");
}

BOOST_AUTO_TEST_CASE(test_args_spaces) {
  std::string name;
  std::map<std::string, std::string> args;
  parse_name_and_parameters("normal(mean=5,  var=3,validate=true)",
                            &name, &args);
  BOOST_TEST(name == "normal");
  BOOST_TEST(args.size() == 3);
  BOOST_TEST(args["mean"] == "5");
  BOOST_TEST(args["var"] == "3");
  BOOST_TEST(args["validate"] == "true");
}
