// Copyright 2024
// See LICENSE.txt

#pragma once

#include "distributions/bigram.hh"

// A distribution over natural numbers represented as strings of digits.
// Good for things like zipcodes where keeping the leading 0's is important.
// Also, vastly more numerically stable than Skellam for sets of natural
// numbers that span may orders of magnitude.
class StringNat : public Bigram {
 public:
  StringNat(size_t _max_length = 20): Bigram(_max_length, '0', '9') {}
};
