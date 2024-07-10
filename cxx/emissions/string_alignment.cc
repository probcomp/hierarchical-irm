#include "emissions/string_alignment.hh"

// Our heap will hold (-cost, alignment) pairs.
using HeapElement = std::pair<double, StrAlignment>;

void topn_alignments(int n, const std::string& s1, const std::string& s2,
                     CostFunction cost_function,
                     std::vector<StrAlignment>* alignments) {
  std::vector<HeapElement> heap;
  StrAlignment empty_alignment;

  heap.push_back(std::make_pair<double, StrAlignment>(0.0, empty_alignment));
  int num_found = 0;

  while ((num_found < n) && heap.size() > 0) {
    // Pop the lowest cost element off the heap.
    auto heap_top = heap.front();
    std::pop_heap(heap.begin(), heap.end());
    std::pop_back();

    // Does this alignment reach the end of both strings?
    double old_cost = -heap_top.first;
    StrAlignment alignment = heap_top.second;
    if ((alignment.s1_position = s1.length())
        && (alignment.s2_position = s2.length())) {
      alignments.push_back(alignment);
      ++num_found;
      continue;
    }

    // Push all the single piece continuations of alignment onto the heap.
    StrAlignment del(alignment);
    Deletion d;
    d.deleted_char = s1[del.s1_position++];
    del.align_pieces.push_back(d);
    double del_cost = cost_function(del, old_cost);
    heap.push_back(std::make_pair<double, StrAlignment>(-del_cost, del));
    std::push_heap(heap.begin(), heap.end());

    StrAlignment ins(alignment);
    Insertion i;
    i.inserted_char = s2[ins.s2_position++];
    ins.align_pieces.push_back(i);
    double ins_cost = cost_function(ins, old_cost);
    heap.push_back(std::make_pair<double, StrAlignment>(-ins_cost, ins));
    std::push_heap(heap.begin(), heap.end());

    StrAlignment next(alignment);
    char c1 = s1[next.s1_position++];
    char c2 = s2[next.s2_position++];
    if (c1 == c2) {
      Match m;
      m.c = c1;
      next.align_pieces.push_back(m);
    } else {
      Substitution s;
      s.original = c1;
      s.replacement = c2;
      next.align_pieces.push_back(s);
    }
    double next_cost = cost_function(next, old_cost);
    heap.push_back(std::make_pair<double, StrAlignment>(-next_cost, next));
    std::push_heap(heap.begin(), heap.end());
  }
}

double edit_distance(const StrAlignment& alignment, double old_cost) {
  const AlignPiece& p = alignment.align_pieces.back();
  if (p.index() == 3) { // Match
    return old_cost;
  } else {
    return old_cost + 1;
  }
}
