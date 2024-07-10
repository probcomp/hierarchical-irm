#pragma once

#include <functional>
#include <string>
#include <variant>
#include <vector>

struct Deletion {
  char deleted_char;
};

struct Insertion {
  char inserted_char;
};

struct Substitution {
  char original;
  char replacement;
};

struct Match {
  char c;
};

using AlignPiece = std::variant<Deletion, Insertion, Substitution, Match>;

class StrAlignment {
 public:
  int s1_position;
  int s2_position;
  std::vector<AlignPiece> align_pieces;
};

// A CostFunction takes a string alignment and the previous cost (i.e., the
// cost of the StrAlignment minus its last AlignPiece) and returns the new
// cost of the alignment.
using CostFunction = std::function<double(const StrAlignment&, double)>;

// Put the top-n lowest cost alignments between s1 and s2 into *alignments.
void topn_alignments(int n, const std::string& s1, const std::string& s2,
                     CostFunction cost_function,
                     std::vector<StrAlignment>* alignments);

double edit_distance(const StrAlignment& alignment, double old_cost);
