#define BOOST_TEST_MODULE test pclean_parse

#include "pclean/parse.hh"

#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_tokenize) {
  std::vector<Token> tokens;
  BOOST_TEST(tokenize("this is a test line", &tokens));
  BOOST_TEST(tokens.size() == 5);
  BOOST_CHECK(tokens[0].type == TokenType::identifier);
}

BOOST_AUTO_TEST_CASE(test_tokenize_types) {
  std::vector<Token> tokens;
  BOOST_TEST(tokenize("a=1+2;", &tokens));
  BOOST_TEST(tokens.size() == 6);
  BOOST_CHECK(tokens[0].type == TokenType::identifier);
  BOOST_CHECK(tokens[1].type == TokenType::symbol);
  BOOST_CHECK(tokens[2].type == TokenType::number);
  BOOST_CHECK(tokens[3].type == TokenType::symbol);
  BOOST_CHECK(tokens[4].type == TokenType::number);
  BOOST_CHECK(tokens[5].type == TokenType::symbol);
}

BOOST_AUTO_TEST_CASE(test_tokenize_nontokens) {
  std::vector<Token> tokens;
  BOOST_TEST(tokenize("\ta  = 1.+2.00;  # testing\n", &tokens));
  BOOST_TEST(tokens.size() == 6);
  BOOST_CHECK(tokens[0].type == TokenType::identifier);
  BOOST_CHECK(tokens[1].type == TokenType::symbol);
  BOOST_CHECK(tokens[2].type == TokenType::number);
  BOOST_CHECK(tokens[3].type == TokenType::symbol);
  BOOST_CHECK(tokens[4].type == TokenType::number);
  BOOST_CHECK(tokens[5].type == TokenType::symbol);
}

BOOST_AUTO_TEST_CASE(test_tokenize_bad_string) {
  std::vector<Token> tokens;
  BOOST_TEST(!tokenize("a = 'unterminated string", &tokens));
}

BOOST_AUTO_TEST_CASE(test_tokenize_good_string) {
  std::vector<Token> tokens;
  BOOST_TEST(tokenize("'a', 'b c d', 'e+f.g 02 <>'", &tokens));
  BOOST_TEST(tokens.size() == 5);
  BOOST_CHECK(tokens[0].type == TokenType::string);
  BOOST_CHECK(tokens[1].type == TokenType::symbol);
  BOOST_CHECK(tokens[2].type == TokenType::string);
  BOOST_CHECK(tokens[3].type == TokenType::symbol);
  BOOST_CHECK(tokens[4].type == TokenType::string);
}
