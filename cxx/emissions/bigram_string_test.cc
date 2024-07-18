// Apache License, Version 2.0, refer to LICENSE.txt

#define BOOST_TEST_MODULE test bigram_string

#include "emissions/bigram_string.hh"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_get_index) {
  BigramStringEmission bse;
  int num_contexts = 2 + (bse.highest_char - bse.lowest_char);
  for (int i = bse.lowest_char; i <= bse.highest_char; ++i) {
    size_t j = bse.get_index(char(i));
    BOOST_TEST(j < num_contexts);
    size_t k = bse.get_index(std::string() + char(i));
    BOOST_TEST(j == k);
  }
  BOOST_TEST(bse.get_index("") == 0);
}

BOOST_AUTO_TEST_CASE(test_category_to_char) {
  BigramStringEmission bse;
  int num_contexts = 2 + (bse.highest_char - bse.lowest_char);
  for (int i = 0; i < num_contexts; ++i) {
    BOOST_TEST(bse.get_index(bse.category_to_char(i)) == i);
  }
}

BOOST_AUTO_TEST_CASE(test_log_prob_distance) {
  BigramStringEmission bse;

  // Test that matches have a lower cost than subsitutions, deletions, or
  // insertions, because otherwise the string alignment algorithm can go
  // haywire.
  StrAlignment a1;
  Match m;
  m.c = 'a';
  a1.align_pieces.push_back(m);
  double match_cost = bse.log_prob_distance(a1, 0.0);

  StrAlignment a2;
  Substitution s;
  s.original = 'a';
  s.replacement = 'b';
  a2.align_pieces.push_back(s);
  double sub_cost = bse.log_prob_distance(a2, 0.0);
  BOOST_TEST(match_cost < sub_cost);

  StrAlignment a3;
  Deletion d;
  d.deleted_char = 'a';
  a3.align_pieces.push_back(d);
  double del_cost = bse.log_prob_distance(a3, 0.0);
  BOOST_TEST(match_cost < del_cost);

  StrAlignment a4;
  Insertion i;
  i.inserted_char = 'a';
  a4.align_pieces.push_back(i);
  double ins_cost = bse.log_prob_distance(a4, 0.0);
  BOOST_TEST(match_cost < ins_cost);
}

BOOST_AUTO_TEST_CASE(test_incorporate) {
  BigramStringEmission bse;
  double lp1 = bse.logp({"hi", "hi"}); // crashes here
  bse.incorporate({"bye", "bye"});
  BOOST_TEST(bse.N == 1);
  bse.unincorporate({"bye", "bye"});
  BOOST_TEST(bse.N == 0);
  double lp2 = bse.logp({"hi", "hi"});
  BOOST_TEST(lp1 == lp2);
}

BOOST_AUTO_TEST_CASE(test_sample_corrupted) {
  BigramStringEmission bse;
  std::mt19937 prng;
  // Incorporate some clean data.
  bse.incorporate({"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz"});
  std::string corrupted = bse.sample_corrupted("test", &prng);
  printf("corrupted string = %s\n", corrupted.c_str());
  BOOST_TEST(corrupted.length() < 7);
}

BOOST_AUTO_TEST_CASE(test_propose_clean) {
  BigramStringEmission bse;
  std::mt19937 prng;
  BOOST_TEST(bse.propose_clean({"clean"}, &prng) == "clean");
  BOOST_TEST(
      bse.propose_clean({"clean", "clean!", "cl5an", "lean"}, &prng)
      == "clean");
}
