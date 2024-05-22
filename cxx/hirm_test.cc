// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test BetaBernoulli

#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/test/included/unit_test.hpp>
#include "hirm.hh"
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_crp)
{
  PRNG prng;
  auto crp = CRP(&prng);
  double alpha = 1.;
  crp.incorporate(0, 0);
  crp.incorporate(1, 1);
  crp.incorporate(2, 1);
  crp.incorporate(3, 0);
  crp.incorporate(4, 0);
  crp.incorporate(5, 2);
  // Check that the table weights reflect the number of people at each table.
  auto weights = crp.tables_weights();
  BOOST_CHECK_CLOSE(weights[0], 3., 1e-6);
  BOOST_CHECK_CLOSE(weights[1], 2., 1e-6);
  BOOST_CHECK_CLOSE(weights[2], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[3], alpha, 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(0), log(3. / (6. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(1), log(2. / (6. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(2), log(1. / (6. + alpha)), 1e-6);

  // Delete two people and check again.
  crp.unincorporate(0);
  crp.unincorporate(2);

  weights = crp.tables_weights();
  BOOST_CHECK_CLOSE(weights[0], 2., 1e-6);
  BOOST_CHECK_CLOSE(weights[1], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[2], 1., 1e-6);
  BOOST_CHECK_CLOSE(weights[3], alpha, 1e-6);

  BOOST_CHECK_CLOSE(crp.logp(0), log(2. / (4. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(1), log(1. / (4. + alpha)), 1e-6);
  BOOST_CHECK_CLOSE(crp.logp(2), log(1. / (4. + alpha)), 1e-6);
}
