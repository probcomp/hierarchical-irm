// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test string_alignment

#include "emissions/string_alignment.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_simple) {
  std::vector<StrAlignment> alignments;
  topn_alignments(1, "hello", "world", edit_distance, &alignments);
  BOOST_TEST(alignments.size() == 1);
  BOOST_TEST(alignments[0].s1_position == 5);
  BOOST_TEST(alignments[0].s2_position == 5);
  BOOST_TEST(alignments[0].align_pieces.size() == 5);
  BOOST_TEST(alignments[0].align_pieces[0].index() == 2);
  BOOST_TEST(alignments[0].align_pieces[1].index() == 2);
  BOOST_TEST(alignments[0].align_pieces[2].index() == 2);
  BOOST_TEST(alignments[0].align_pieces[3].index() == 3);
  BOOST_TEST(alignments[0].align_pieces[4].index() == 2);
}
