// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test relation_variant

#include "relation_variant.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_relation_variant) {
  std::vector<Domain *> domains;
  domains.push_back(new Domain("D1"));
  RelationVariant rv = relation_from_spec(
      "r1", parse_distribution_spec("bernoulli"), domains);
  Relation<bool>* rb = std::get<Relation<bool>*>(rv);
  BOOST_TEST(rb->name == "r1");
}
