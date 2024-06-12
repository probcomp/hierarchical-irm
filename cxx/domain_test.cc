// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test Domain

#include "domain.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_domain) {
  std::mt19937 prng;
  Domain d("fruit", &prng);
  std::string relation1 = "grows_on_trees";
  std::string relation2 = "is_same_color";
  T_item banana = 1;
  T_item apple = 2;
  d.incorporate(banana);
  d.set_cluster_assignment_gibbs(banana, 12);
  d.incorporate(banana);
  d.incorporate(apple, 5);
  BOOST_TEST(d.items.contains(banana));
  BOOST_TEST(d.items.contains(apple));
  BOOST_TEST(d.items.size() == 2);

  int cb = d.get_cluster_assignment(banana);
  int ca = d.get_cluster_assignment(apple);
  BOOST_TEST(ca == 5);
  BOOST_TEST(cb == 12);

}