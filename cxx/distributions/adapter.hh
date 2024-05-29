// Copyright 2024
// See LICENSE.txt

// A class for turning Distribution<SampleType>'s into
// Distribution<string>'s.

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include "distributions/base.hh"
#include "distributions/beta_bernoulli.hh"

Distribution<std::string>* get_distribution(const std::string& dist_str);

template <typename SampleType = double> 
class DistributionAdapter : public Distribution<std::string> {
public:
    // The underlying distribution that is being adapted.
    Distribution<SampleType> *d;

    DistributionAdapter(Distribution<SampleType> *dd): d(dd) {
      // THIS PRINTS BETABERNOULLI, AND INCORPORATE PRINTS DISTRIBUTION
      std::cout << "constructing, \n  d: " << typeid(*d).name() << "\n  dd: " << typeid(*dd).name() << std::endl;
    };

    SampleType from_string(const std::string& x) const {
      SampleType s;
      std::istringstream(x) >> s;
      return s;
    }

    std::string to_string(const SampleType& s) const {
      std::ostringstream os;
      os << s;
      return os.str();
    }

    void incorporate(const std::string& x) {
      std::cerr << "in adapter incorporate first" << std::endl;
      SampleType s = from_string(x);
      std::cerr << "in adapter incorporate next" << std::endl;
      std::cerr << "in adapter incorporate s " << s << std::endl;
      std::cerr << "in adapter incorporate s type " << typeid(s).name() << std::endl;

      // THIS PRINTS DISTRIBUTION, AND THE CONSTRUCTOR PRINTS BETABERNOULLI
      // https://stackoverflow.com/questions/17514243/unable-to-call-derived-class-function-using-base-class-pointers
      std::string my_name = typeid(*d).name();
      std::cerr << "in adapter incorporate, dist name is " << my_name << std::endl;
      const double cd = double(s);
      // BetaBernoulli* dBB = dynamic_cast<BetaBernoulli*>(d);
      d->incorporate(cd);
      // `virtual` derived method: https://stackoverflow.com/questions/18885224/how-to-call-derived-class-method-from-base-class-pointern
      // dBB->incorporate(cd);  // seg fault
      // d->incorporate_BB(cd);
      std::cerr << "in adapter incorporate returning" << std::endl;
    }

    void unincorporate(const std::string& x) {
      SampleType s = from_string(x);
      d->unincorporate(s);
    }

    double logp(const std::string& x) const {
      SampleType s = from_string(x);
      return d->logp(s);
    }

    double logp_score() const {
      return d->logp_score();
    }

    std::string sample() {
      SampleType s = d->sample();
      return to_string(s);
    }
};
