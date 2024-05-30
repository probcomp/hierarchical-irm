#pragma once

#include "emissions/base.hh"

class GaussianEmission : public Emission {
  // We use an Inverse gamma conjugate prior, so our hyperparameters are
  double alpha = 1.0;
  double beta = 1.0;

  // Sufficient statistics of observed data.
  double mean = 0.0;
  double var = 0.0;


}
