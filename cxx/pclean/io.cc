// Copyright 2024
// Apache License, Version 2.0, refer to LICENSE.txt

#include <fstream>

#include "pclean/io.hh"

#include "util_parse.hh"

bool read_schema_file(const std::string& path, PCleanSchema* schema) {
  std::ifstream f(path);

  if (!f.is_open()) {
    printf("Unable to open PClean schema file %s\n", path.c_str());
    return false;
  }

  bool success = read_schema(f, schema);

  f.close();

  return success;
}


bool resolve_variable_definition(const std::vector<Token> &tokens,
                                 PCleanVariable* pcv) {
  if ((tokens.size() == 3) && (std::isupper(tokens[2].val[0]))) {
    // Class variable
    ClassVar cv;
    cv.class_name = tokens[2].val;
    pcv->spec = cv;
    return true;
  }

  bool is_emission = false;
  for (size_t i = 3; i < tokens.size(); ++i) {
    if (tokens[i].val == "(") {
      is_emission = true;
      break;
    }
  }

  if (is_emission) {
    EmissionVar ev;
    ev.emission_name = tokens[2].val;

    size_t i = 3;
    if (tokens[i].val == "[") {  // Emission parameters
      i += 1;
      while (tokens[i + 3].val == ",") {
        if (tokens[i + 1].val != "=") {
          printf("Expected '=' in declaration of emission variable %s\n", tokens[0].val.c_str());
          return false;
        }
        ev.emission_params[tokens[i].val] = tokens[i+2].val;
        i += 4;
      }
    }

    if (tokens[i].val != "(") {  // Field path
      printf("Expected '(' but got %s in declaration of emission variable %s\n", tokens[i].val.c_str(), tokens[0].val.c_str());
      return false;
    }

    do {
      i += 1;
      if (tokens[i].type != TokenType::identifier) {
        printf("Expected identifier but got %s in declaration of emission variable %s\n", tokens[i].val.c_str(), tokens[0].val.c_str());
        return false;
      }
      ev.field_path.push_back(tokens[i++].val);
    } while (tokens[i].val == ".");

    if (tokens[i].val != ")") {
      printf("Expected ')' but got %s in declaration of emission variable %s\n",
             tokens[i].val.c_str(), tokens[0].val.c_str());
      printf("emission name = %s\n", ev.emission_name.c_str());
      return false;
    }

    pcv->spec = ev;
    return true;
  }

  DistributionVar dv;
  dv.distribution_name = tokens[2].val;
  for (size_t i = 4; i < tokens.size() - 1; i += 4) {
    if (i + 3 > tokens.size()) {
      printf("Error parsing parameters of distribution variable %s -- too few tokens\n", tokens[0].val.c_str());
      printf("At token %ld of %ld\n", i, tokens.size());
      return false;
    }
    if (tokens[i+1].val != "=") {
      printf("Error parsing parameters of distribution variable %s -- expected '=' but got %s\n", tokens[0].val.c_str(), tokens[i+1].val.c_str());
      return false;
    }
    std::string s = tokens[i+3].val;
    if ((s != ",") && (s != "]")) {
      printf("Error parsing parameters of distribution variable %s -- expected ',' or '] but got %s'\n", tokens[0].val.c_str(), s.c_str());
      return false;
    }
    dv.distribution_params[tokens[i].val] = tokens[i+2].val;
  }
  pcv->spec = dv;

  return true;
}

// We expect a class to look like
// class ClassName                       # Should start with an uppercase char!
//   name ~ categorical[num_classes=5]   # distribution variable
//   city ~ City                         # class variable
//   bad_city ~ typos(city.name)         # emission variable
//
// ^ class definition ends with a blank line (or end of file)
bool read_class(std::istream& is, PCleanClass* pclass) {
  std::string line;
  std::vector<Token> tokens;
  while (std::getline(is, line)) {
    if (line.empty()) {
      return true;
    }
    tokens.clear();
    if (!tokenize(line, &tokens)) {
      printf("Error tokenizing line %s\n", line.c_str());
      return false;
    }
    if (tokens.empty()) {
      continue;
    }

    if (tokens.size() < 3) {
      printf("Error parsing class variable line %s:  expected at least three tokens.\n", line.c_str());
      return false;
    }

    if (tokens[1].val != "~") {
      printf("Error parsing class variable line %s:  expected second token to be '~'.\n", line.c_str());
      return false;
    }

    PCleanVariable v;
    v.name = tokens[0].val;

    bool success = resolve_variable_definition(tokens, &v);
    if (!success) {
      printf("on line %s\n", line.c_str());
      return false;
    }

    pclass->vars.push_back(v);
  }
  return true;
}

// We expect that a query looks like
// observe
//   physician.speciality as Specialty
//   physician.school.name as School
//   physician.observed_degree as Degree
//   location.city.state as State
//   from Record
//
// ^ query ends with a blank line or end of file.
bool read_query(std::istream& is, PCleanQuery* query) {
  std::string line;
  std::vector<Token> tokens;
  while (std::getline(is, line)) {
    if (line.empty()) {
      return true;
    }
    tokens.clear();
    if (!tokenize(line, &tokens)) {
      printf("Error tokenizing line %s\n", line.c_str());
      return false;
    }
    if (tokens.empty()) {
      continue;
    }

    if (tokens[0].val == "from") {
      if (tokens.size() != 2) {
        printf("Expected exactly two tokens on query from line %s", line.c_str());
        return false;
      }
      if (!query->base_class.empty()) {
        printf("Expected exactly one `from` clause in query.\n");
        return false;
      }
      query->base_class = tokens[1].val;
      continue;
    }

    if (tokens.size() < 3) {
      printf("Expected at least three tokens on `as` query line %s", line.c_str());
      return false;
    }

    QueryField qf;
    qf.name = tokens.back().val;
    for (size_t i = 0; i < tokens.size() - 2; ++i) {
      if (i % 2 == 0) {
        qf.class_path.push_back(tokens[i].val);
      } else {
        if (tokens[i].val != ".") {
          printf("Expected . at token %ld on query line %s\n", i, line.c_str());
          return false;
        }
      }
    }
    query->fields.push_back(qf);
  }
  return true;
}

bool read_schema(std::istream& is, PCleanSchema* schema) {
  std::string line;
  std::vector<Token> tokens;
  while (std::getline(is, line)) {
    tokens.clear();
    if (!tokenize(line, &tokens)) {
      printf("Error tokenizing line %s\n", line.c_str());
      return false;
    }

    if (tokens.empty()) {
      continue;
    }

    if (tokens[0].type != TokenType::identifier) {
      printf("Error parsing line %s:  expected `class` or `query` as first token\n", line.c_str());
      printf("Got %s of type %d instead\n",
             tokens[0].val.c_str(), (int)(tokens[0].type));
      return false;
    }

    if (tokens[0].val == "class") {
      if (tokens.size() != 2) {
        printf("Error:  expected exactly two tokens on class line %s\n",
               line.c_str());
        return false;
      }
      PCleanClass pcc;
      if (tokens[0].type != TokenType::identifier) {
        printf("Error parsing line %s:  second token of a class line must be an identifier\n",
               line.c_str());
        return false;
      }
      pcc.name = tokens[1].val;
      if (!std::isupper(pcc.name[0])) {
        printf("Warning!  Class names should start with an uppercase character:  %s\n", line.c_str());
      }
      bool success = read_class(is, &pcc);
      if (!success) {
        return false;
      }
      schema->classes.push_back(pcc);
      continue;
    }

    if (tokens[0].val == "observe") {
      if (!schema->query.base_class.empty()) {
        printf("Error reading schema line %s:  only one query is allowed\n",
               line.c_str());
        return false;
      }
      if (tokens.size() != 1) {
        printf("Error:  expected exactly one tokens on query line %s\n",
               line.c_str());
        return false;
      }
      bool success = read_query(is, &(schema->query));
      if (!success) {
        return false;
      }
      continue;
    }

    printf("Error parsing line %s:  expected `class` or `query` as first "
           "token\n", line.c_str());
    printf("Got %s of type %d instead\n",
             tokens[0].val.c_str(), (int)(tokens[0].type));
    return false;
  }
  return true;
}
