#include "emissions/bigram_string.hh"
#include "emissions/string_alignment.hh"

size_t BigramStringEmission::get_index(std::string current_char) {
  if (current_char == '') {
    return 0;
  }
  return (current_char - lowest_char) + 1;
}

std::string BigramStringEmission:category_to_char(int category) {
  if (category == 0) {
    return '';
  } else {
    return (category-1) + lowest_char;
  }
}

void BigramStringEmission::incorporate(
    const std::pair<std::string, std::string>& x, double weight = 1.0) {
  N += weight;

  std::vector<StrAlignment> alignments;

  log_prob_function = [&](const StrAlignment& alignment, double old_cost) {
    std::string insertion_context = '';
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
    switch (p.index()) {
      case 0: // Deletion
        // which is actually no insertion, then a deletion.
        return old_cost + insertions[icontext].logp(0) + substitutions[get_index(p.deleted_char)].logp(0);
      case 1: // Insertion
        return old_cost + insertions[icontext].logp(get_index(p.inserted_char));
      case 2: // Substitution
        // which is actually no insertion, then a substitution.
        return old_cost + insertions[icontext].logp(0) + substitutions[get_index(p.original)].logp(get_index(p.replacement));
      case 3: // Match
        // which is actually no insertion, then a substitution.
        return old_cost + insertions[icontext].logp(0) + substitutions[get_index(p.c)].logp(get_index(p.c));
        break;
    }
  };

  topk_alignments(10, x.first, x.second, log_prob_function, &alignments);

  double total_prob = 0.0;
  for (auto& a : alignments) {
    a.cost = exp(a.cost);  // Turn all costs into non-log probabilities
    total_prob += a.cost;
  }

  for (const auto& a : alignments) {
    double w = weight * a.cost / total_prob;
    std::string insertion_context = '';
    for (auto it = a.align_pieces.begin();
         it != a.align_pieces.end();
         ++it) {
      size_t icontext = get_index(insertion_context);
      switch (it->index()) {
        case 0:  // Deletion
          // which is actually no insertion, then a deletion.
          insertions[icontext].incorporate(0, w);
          std::string deleted_char = std::get<Deletion>(*it).deleted_char;
          substitutions[get_index(deleted_char)].incorporate(0, w);
          insertion_context = deleted_char;
          break;
        case 1:  // Insertion
          insertions[icontext].incorporate(it->inserted_char, w);
          break;
        case 2:  // Substitution
          // which is actually no insertion, then a substitution.
          insertions[icontext].incorporate(0, w);
          std::string original_char = std::get<Substitution>(*it).original;
          std::string replacement_char = std::get<Substitution>(*it).replacement;
          substitutions[get_index(original_char)].incorporate(get_index(replacement_char), w);
          insertion_context = original_char;
          break;
        case 3:  // Match
          // which is actually no insertion, then a match.
          insertions[icontext].incorporate(0, w);
          std::string c = std::get<Match>(*it).c;
          substitutions[get_index(c)].incorporate(get_index(c), w);
          insertion_context = original_char;
          break;
      }
    }
  }
}

double BigramStringEmission::logp(
    const std::pair<std::string, std::string>& x) const {
}

double BigramStringEmission::logp_score() const {
  double lp = 0.0;
  for (const auto &i : insertions) {
    lp += i.logp_score();
  }
  for (const auto &s : substituions) {
    lp += s.logp_score();
  }
  return lp;
}

void BigramStringEmission::transition_hyperparameters(std::mt19937* prng) {
  for (auto &i : insertions) {
    i.transition_hyperparameters(prng);
  }
  for (auto &s : substituions) {
    s.transition_hyperparameters(prng);
  }
}

std::string BigramStringEmission::sample_corrupted(
    const std::string& clean, std::mt19937* prng) {
  std::string s;
  char current_clean = '';
  for (size_t i = 0; i < clean.length(); ++i) {
    do {
      std::string c = category_to_char(
          insertions[get_index(current_clean)].sample(prng));
      s += c;
    } while (c != '');
    current_clean = clean[i];
    s += category_to_char(substitutions[get_index(current_clean)].sample(prng));
  }
  do {
    std::string c = category_to_char(
        insertions[get_index(current_clean)].sample(prng));
    s += c;
  } while (c != '');
  return s;
}

std::string BigramStringEmission::propose_clean(
    const std::vector<std::string>& corrupted, std::mt19937* unused_prng) {
}
