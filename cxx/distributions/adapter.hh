// Copyright 2024
// See LICENSE.txt

// A class for turning Distribution<SampleType>'s into
// Distribution<string>'s.

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include "distributions/base.hh"

template <typename SampleType = double> class DistributionAdapter : Distribution<string> {
public:
    // The underlying distribution that is being adapted.
    Distribution<SampleType> *d;

    DistributionAdapter(Distribution<SampleType> *dd): d(dd) {};

    SampleType from_string(const string& x) const {
      SampleType s;
      std::istringstream(x) >> s;
      return s;
    }

    string to_string(const SampleType& s) const {
      std::ostringstream os;
      os << s;
      return os.str();
    }

    void incorporate(const string& x) {
      SampleType s = from_string(x);
      d->incorporate(s);
    }

    void unincorporate(const string& x) {
      SampleType s = from_string(x);
      d->unincorporate(s);
    }

    double logp(const string& x) const {
      SampleType s = from_string(x);
      return d->logp(s);
    }

    double logp_score() const {
      return d->logp_score();
    }

    string sample() {
      SampleType s = d->sample();
      return to_string(s);
    }
};
