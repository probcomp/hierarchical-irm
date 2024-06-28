// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test StringCat

#include "distributions/stringcat.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_simple) {
  StringCat sc({"hello", "world", "train", "test", "other"});

  sc.incorporate("hello");
  sc.incorporate("world");
  BOOST_TEST(sc.N == 2);
  sc.unincorporate("hello");
  BOOST_TEST(sc.N == 1);
  sc.incorporate("train");
  sc.unincorporate("world");
  BOOST_TEST(sc.N == 1);

  BOOST_TEST(sc.logp("test") == -1.791759469228055, tt::tolerance(1e-6));
  BOOST_TEST(sc.logp_score() == -1.6094379124341001, tt::tolerance(1e-6));
}
