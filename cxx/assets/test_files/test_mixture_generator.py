# Generates a mixture of 2 normals as an HIRM observation file.
import numpy as np

NUM_SAMPLES = 1000

def main():
  rng = np.random.default_rng(1762)
  left_samples = rng.normal(-1., 0.5, NUM_SAMPLES)
  right_samples = rng.normal(2., 0.5, NUM_SAMPLES)
  samples = np.where(
    rng.random(NUM_SAMPLES) > 0.5, left_samples, right_samples)
  f = open('test_mixture_2_normals.obs', 'w')
  for i in range(NUM_SAMPLES):
    f.write(f"{samples[i]},has_value,{samples[i]}\n")
  f.close()

main()
