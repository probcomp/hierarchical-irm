#include <cassert>

#include "emissions/bigram_string.hh"
#include "emissions/string_alignment.hh"

size_t BigramStringEmission::get_index(std::string current_char) {
  if (current_char == "") {
    return 0;
  }
  assert(current_char.length() == 1);
  return (current_char[0] - lowest_char) + 1;
}

size_t BigramStringEmission::get_index(char current_char) {
  return (current_char - lowest_char) + 1;
}

std::string BigramStringEmission::category_to_char(int category) {
  if (category == 0) {
    return "";
  } else {
    return std::string() + char(lowest_char + (category - 1));
  }
}

double BigramStringEmission::log_prob_distance(const StrAlignment& alignment, double old_cost) {
  std::string insertion_context = "";
  for (auto it = alignment.align_pieces.rbegin();
       it != alignment.align_pieces.rend();
       ++it) {
    switch (it->index()) {
      case 0: // Deletion
        insertion_context = std::get<Deletion>(*it).deleted_char;
        break;
      case 1: // Insertion
        continue;
      case 2: // Substitution
        insertion_context = std::get<Substitution>(*it).original;
        break;
      case 3: // Match
        insertion_context = std::get<Match>(*it).c;
        break;
    }
  }
  size_t icontext = get_index(insertion_context);
  const AlignPiece& p = alignment.align_pieces.back();
  double log_prob = 0.0;
  switch (p.index()) {
    case 0: // Deletion
            // which is actually no insertion, then a deletion.
      {
        Deletion del = std::get<Deletion>(p);
        log_prob = insertions[icontext].logp(0) + substitutions[get_index(del.deleted_char)].logp(0);
      }
      break;
    case 1: // Insertion
      {
        Insertion ins = std::get<Insertion>(p);
        log_prob = insertions[icontext].logp(get_index(ins.inserted_char));
      }
      break;
    case 2: // Substitution
            // which is actually no insertion, then a substitution.
      {
        Substitution sub = std::get<Substitution>(p);
        log_prob = insertions[icontext].logp(0) + substitutions[get_index(sub.original)].logp(get_index(sub.replacement));
      }
      break;
    case 3: // Match
            // which is actually no insertion, then a substitution.
      {
        Match mat = std::get<Match>(p);
        log_prob = insertions[icontext].logp(0) + substitutions[get_index(mat.c)].logp(get_index(mat.c));
      }
      break;
    default:
      assert("Error!  Unknown AlignPiece in log_prob_distance\n");
      break;
  }
  // old_cost is a negative log prob, and the returned cost should also be
  // a negative log prob.
  return old_cost - log_prob;
}

void BigramStringEmission::incorporate(
    const std::pair<std::string, std::string>& x, double weight) {
  N += weight;

  std::vector<StrAlignment> alignments;

  topk_alignments(10, x.first, x.second,
                  [&](const StrAlignment &a, double old_cost) {
                    return log_prob_distance(a, old_cost);
                  },
                  &alignments);

  double total_prob = 0.0;
  for (auto& a : alignments) {
    a.cost = exp(a.cost);  // Turn all costs into non-log probabilities
    total_prob += a.cost;
  }

  for (const auto& a : alignments) {
    double w = weight * a.cost / total_prob;
    std::string insertion_context = "";
    for (auto it = a.align_pieces.begin();
         it != a.align_pieces.end();
         ++it) {
      size_t icontext = get_index(insertion_context);
      switch (it->index()) {
        case 0:  // Deletion
          // which is actually no insertion, then a deletion.
          {
            insertions[icontext].incorporate(0, w);
            char deleted_char = std::get<Deletion>(*it).deleted_char;
            substitutions[get_index(deleted_char)].incorporate(0, w);
            insertion_context = std::string() + deleted_char;
          }
          break;
        case 1:  // Insertion
          {
            char inserted_char = std::get<Insertion>(*it).inserted_char;
            insertions[icontext].incorporate(inserted_char, w);
          }
          break;
        case 2:  // Substitution
          // which is actually no insertion, then a substitution.
          {
            insertions[icontext].incorporate(0, w);
            char original_char = std::get<Substitution>(*it).original;
            char replacement_char = std::get<Substitution>(*it).replacement;
            substitutions[get_index(original_char)].incorporate(get_index(replacement_char), w);
            insertion_context = std::string() + original_char;
          }
          break;
        case 3:  // Match
          // which is actually no insertion, then a match.
          {
            insertions[icontext].incorporate(0, w);
            char c = std::get<Match>(*it).c;
            substitutions[get_index(c)].incorporate(get_index(c), w);
            insertion_context = std::string() + c;
          }
          break;
      }
    }
  }
}

double BigramStringEmission::logp(
    const std::pair<std::string, std::string>& x) const {
  // Store state.
  std::vector<DirichletCategorical> orig_insertions;
  std::vector<DirichletCategorical> orig_substitutions;
  std::copy(insertions.begin(), insertions.end(), orig_insertions.begin());
  std::copy(substitutions.begin(), insertions.end(), orig_insertions.begin());
  double orig_lp = logp_score();
  BigramStringEmission* unsafe_this = const_cast<BigramStringEmission*>(this);
  unsafe_this->incorporate(x);
  double new_lp = logp_score();
  // Restore state
  unsafe_this->insertions.clear();
  std::copy(orig_insertions.begin(), orig_insertions.end(),
            unsafe_this->insertions.begin());
  unsafe_this->substitutions.clear();
  std::copy(orig_substitutions.begin(), orig_substitutions.end(),
            unsafe_this->substitutions.begin());
  return new_lp - orig_lp;
}

double BigramStringEmission::logp_score() const {
  double lp = 0.0;
  for (const auto &i : insertions) {
    lp += i.logp_score();
  }
  for (const auto &s : substitutions) {
    lp += s.logp_score();
  }
  return lp;
}

void BigramStringEmission::transition_hyperparameters(std::mt19937* prng) {
  for (auto &i : insertions) {
    i.transition_hyperparameters(prng);
  }
  for (auto &s : substitutions) {
    s.transition_hyperparameters(prng);
  }
}

std::string BigramStringEmission::sample_corrupted(
    const std::string& clean, std::mt19937* prng) {
  std::string s;
  std::string c;
  std::string current_clean = "";
  for (size_t i = 0; i < clean.length(); ++i) {
    do {
      c = category_to_char(insertions[get_index(current_clean)].sample(prng));
      s += c;
    } while (c != "");
    current_clean = clean[i];
    s += category_to_char(substitutions[get_index(current_clean)].sample(prng));
  }
  do {
    c = category_to_char(insertions[get_index(current_clean)].sample(prng));
    s += c;
  } while (c != "");
  return s;
}

std::string BigramStringEmission::two_string_vote(
    const std::string &s1, const std::string &s2) {
  std::vector<StrAlignment> alignments;
  topk_alignments(1, s1, s2,
                  [&](const StrAlignment &a, double old_cost) {
                    return log_prob_distance(a, old_cost);
                  },
                  &alignments);
  std::string clean = "";
  std::string left_context = "";
  for (const auto& p: alignments[0].align_pieces) {
    std::string new_char = "";
    switch (p.index()) {
      case 0: // Deletion
        {
          const Deletion &del = std::get<Deletion>(p);
          double lp1 = substitutions[get_index(del.deleted_char)].logp(0);
          double lp2 = insertions[get_index(left_context)].logp(get_index(del.deleted_char));
          if (lp1 > lp2) {
            new_char = del.deleted_char;
          }
        }
        break;
      case 1: // Insertion
        {
          const Insertion &ins = std::get<Insertion>(p);
          double lp1 = insertions[get_index(left_context)].logp(get_index(ins.inserted_char));
          double lp2 = substitutions[get_index(ins.inserted_char)].logp(0);
          if (lp2 > lp1) {
            new_char = ins.inserted_char;
          }
        }
        break;
      case 2: // Substitution
        {
          const Substitution &sub = std::get<Substitution>(p);
          double lp1 = substitutions[get_index(sub.original)].logp(get_index(sub.replacement));
          double lp2 = substitutions[get_index(sub.replacement)].logp(get_index(sub.original));
          if (lp1 > lp2) {
            new_char = sub.original;
          } else {
            new_char = sub.replacement;
          }
        }
        break;
      case 3: // Match
        new_char = std::get<Match>(p).c;
        break;
    }
    if (new_char.length() > 0) {
      clean += new_char;
      left_context = new_char;
    }
  }
  return clean;
}

std::string BigramStringEmission::propose_clean(
    const std::vector<std::string>& corrupted, std::mt19937* unused_prng) {
  // One way to solve this would be to align all of the corrupted strings.
  // Sadly, that has time complexity of 2 ^ corrupted.size().  So instead
  // we do a tournament of pairwise string alignments, with the learnt
  // probability model being used to vote for the most likely clean version
  // of the string given the alignment.  That should work well for good
  // probability models, but for flat models (including models without a
  // lot of training data), it would be better to do a tournament of threeway
  // string alignments, because un-weighted voting works better with three
  // voters than with two.
  if (corrupted.empty()) {
    printf("Warning!  propose_clean called with empty corrupted list\n");
    return "";
  }
  if (corrupted.size() == 1) {
    return corrupted[0];
  }
  std::vector<std::string> winners;
  for (size_t i = 0; i < corrupted.size() - 1; i += 2) {
    winners.push_back(two_string_vote(corrupted[i], corrupted[i+1]));
  }
  if (corrupted.size() % 2 == 1) {
    winners.push_back(corrupted.back());
  }
  return propose_clean(winners, unused_prng);
}
