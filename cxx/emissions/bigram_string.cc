#include "emissions/bigram_string.hh"
#include "emissions/string_alignment.hh"

void BigramStringEmission::incorporate(
    const std::pair<std::string, std::string>& x, double weight = 1.0) {
  N += weight;

  std::vector<StrAlignment> alignments;
  topk_alignments(10, x.first, x.second, log_prob_function, &alignments);
}

double BigramStringEmission::logp(
    const std::pair<std::string, std::string>& x) const {
}

double BigramStringEmission::logp_score() const {
}

void BigramStringEmission::transition_hyperparameters(std::mt19937* prng) {
}

size_t BigramStringEmission::get_index(char current_char) {
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
    if (deletions[get_index(current_clean)].sample(prng)) {
      continue;
    }
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
