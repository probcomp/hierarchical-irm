#include <utility>
#include "emissions/string_alignment.hh"

void topk_alignments(int k, const std::string& s1, const std::string& s2,
                     CostFunction cost_function,
                     std::vector<StrAlignment>* alignments) {
  std::vector<StrAlignment> heap;
  StrAlignment empty_alignment;
  empty_alignment.cost = -0.0;  // We negate all costs on the heap so that
                                // the front of the heap is the min cost
                                // element.
  empty_alignment.s1_position = 0;
  empty_alignment.s2_position = 0;
  heap.push_back(empty_alignment);

  int num_found = 0;

  while ((num_found < k) && heap.size() > 0) {
    // Pop the lowest cost element off the heap.
    StrAlignment heap_top(heap.front());
    std::pop_heap(heap.begin(), heap.end());
    heap.pop_back();

    // Does this alignment reach the end of both strings?  If so, ship it.
    double old_cost = -heap_top.cost;
    if (std::cmp_equal(heap_top.s1_position, s1.length())
        && std::cmp_equal(heap_top.s2_position, s2.length())) {
      heap_top.cost = old_cost;
      alignments->push_back(heap_top);
      ++num_found;
      continue;
    }

    // Push all the single piece continuations of alignment onto the heap.
    StrAlignment del(heap_top);
    Deletion d;
    d.deleted_char = s1[del.s1_position++];
    del.align_pieces.push_back(d);
    del.cost = -cost_function(del, old_cost);
    heap.push_back(del);
    std::push_heap(heap.begin(), heap.end());

    StrAlignment ins(heap_top);
    Insertion i;
    i.inserted_char = s2[ins.s2_position++];
    ins.align_pieces.push_back(i);
    ins.cost = -cost_function(ins, old_cost);
    heap.push_back(ins);
    std::push_heap(heap.begin(), heap.end());

    StrAlignment next(heap_top);
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
    next.cost = -cost_function(next, old_cost);
    heap.push_back(next);
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
