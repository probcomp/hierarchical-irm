#pragma once

#include <string>
#include <vector>

enum class TokenType { identifier, symbol, number, string };

struct Token {
  TokenType type;
  std::string val;
};

// Parse line into *tokens.  Returns true on success, false on failure.
bool tokenize(std::string line, std::vector<Token>* tokens);
