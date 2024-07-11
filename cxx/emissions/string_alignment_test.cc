// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test string_alignment

#include "emissions/string_alignment.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_simple) {
  std::vector<StrAlignment> alignments;
  topk_alignments(1, "hello", "world", edit_distance, &alignments);
  BOOST_TEST(alignments.size() == 1);
  BOOST_TEST(alignments[0].cost == 4);  // 4 substitutions, 1 match
  BOOST_TEST(alignments[0].s1_position == 5);
  BOOST_TEST(alignments[0].s2_position == 5);
  BOOST_TEST(alignments[0].align_pieces.size() == 5);
  BOOST_TEST(alignments[0].align_pieces[0].index() == 2);  // Substitution
  BOOST_TEST(alignments[0].align_pieces[1].index() == 2);  // Substitution
  BOOST_TEST(alignments[0].align_pieces[2].index() == 2);  // Substitution
  BOOST_TEST(alignments[0].align_pieces[3].index() == 3);  // Match
  BOOST_TEST(alignments[0].align_pieces[4].index() == 2);  // Substitution
}

BOOST_AUTO_TEST_CASE(test_lookahead) {
  std::vector<StrAlignment> alignments;
  topk_alignments(1, "world", "w0rld!", edit_distance, &alignments);
  BOOST_TEST(alignments.size() == 1);
  BOOST_TEST(alignments[0].cost == 2);  // 4 match, 1 substitution, 1 insertion
  BOOST_TEST(alignments[0].s1_position == 5);
  BOOST_TEST(alignments[0].s2_position == 6);
  BOOST_TEST(alignments[0].align_pieces.size() == 6);
  BOOST_TEST(alignments[0].align_pieces[0].index() == 3);  // Match w
  BOOST_TEST(alignments[0].align_pieces[1].index() == 2);  // Substitution
  BOOST_TEST(alignments[0].align_pieces[2].index() == 3);  // Match r
  BOOST_TEST(alignments[0].align_pieces[3].index() == 3);  // Match l
  BOOST_TEST(alignments[0].align_pieces[4].index() == 3);  // Match d
  BOOST_TEST(alignments[0].align_pieces[5].index() == 1);  // Insertion !
}

BOOST_AUTO_TEST_CASE(test_topk) {
  std::vector<StrAlignment> alignments;
  topk_alignments(5, "world", "w0rld!", edit_distance, &alignments);
  BOOST_TEST(alignments.size() == 5);

  for (int i = 0; i < 5; ++i) {
    BOOST_TEST(alignments[i].s1_position == 5);
    BOOST_TEST(alignments[i].s2_position == 6);
    BOOST_TEST(alignments[i].align_pieces.size() < 8);
    BOOST_TEST(alignments[i].align_pieces[0].index() == 3);  // Match w
    // Costs should be non-decreasing.
    if (i > 0) {
      BOOST_TEST(alignments[i].cost >= alignments[i-1].cost);
    }
  }
}

double bio_edit_distance(const StrAlignment& alignment, double old_cost) {
  const AlignPiece& p = alignment.align_pieces.back();
  switch(p.index()) {
    case 3: // Match
      return old_cost;
    case 2: // Substitution
      return old_cost + 1;
    default: // Insertion or Deletion
      return old_cost + 2;
  }
}

BOOST_AUTO_TEST_CASE(test_custom_edit_distance) {
  std::vector<StrAlignment> alignments;
  topk_alignments(5, "AACAGTTACC", "TAAGGTCA", bio_edit_distance, &alignments);
  BOOST_TEST(alignments.size() == 5);
  BOOST_TEST(alignments[0].cost == 7);  // 2 deletions, 3 substitutions
}

