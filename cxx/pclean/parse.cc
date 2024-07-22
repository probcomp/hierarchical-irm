#include "parse.hh"

#include <cctype>
#include <cstring>

#define SYMBOL_CHARS "~()[]{}<>.,;=+*-/"
bool tokenize(std::string line, std::vector<Token>* tokens) {
  size_t i = 0;
  while (i < line.length()) {
    char c = line[i++];
    if (c == '#') { // Comment
      return true;
    }

    if (std::isspace(c)) {
      continue;
    }

    Token t;
    if (std::strchr(SYMBOL_CHARS, c) != NULL) {  // Symbols
      t.type = TokenType::symbol;
      t.val += c;
    }

    if ((c == '"') || (c == '\'')) {  // Strings
      t.type = TokenType::string;
      char sc;
      while (i < line.length()) {
        sc = line[i++];
        if (sc == c) { // End of string
          break;
        }
        t.val += sc;
      }
      if (sc != c) {
        printf("Unclosed string token while parsing line %s", line.c_str());
        return false;
      }
    }

    if (isdigit(c) || (c == '.')) {  // Numbers
      // TODO(thomaswc): Handle exponential notation like "1.2e-5".
      t.type = TokenType::number;
      t.val += c;
      while (i < line.length()) {
        c = line[i];
        bool still_numeric = isdigit(c) || (c == '.') || (c == '_');
        if (!still_numeric) {
          break;
        }
        i++;
        t.val += c;
      }
    }

    if (isalnum(c) || (c == '_')) {  // Identifiers
      t.type = TokenType::identifier;
      while (i < line.length()) {
        c = line[i];
        bool still_id = isalnum(c) || (c == '_');
        if (!still_id) {
          break;
        }
        i++;
        t.val += c;
      }
    }
    tokens->push_back(t);
  }
  return true;
}
