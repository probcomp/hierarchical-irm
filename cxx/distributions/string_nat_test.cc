// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test StringNat

#include "distributions/string_nat.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_simple) {
  StringNat sn;

  sn.incorporate("42");
  sn.incorporate("0");
  BOOST_TEST(sn.N == 2);
  sn.unincorporate("0");
  BOOST_TEST(sn.N == 1);
}

BOOST_AUTO_TEST_CASE(test_nearest) {
  StringNat sn;

  BOOST_TEST(sn.nearest("1234") == "1234");
  BOOST_TEST(sn.nearest("a77z99") == "7799");
}
