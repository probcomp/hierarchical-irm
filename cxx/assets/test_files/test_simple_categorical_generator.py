# Generates a mixture of 2 normals as an HIRM observation file.
import numpy as np

NUM_SAMPLES = 1000

def main():
  rng = np.random.default_rng(1762)
  f = open('test_simple_categorical.obs', 'w')
  for i in range(NUM_SAMPLES):
    if i % 2 == 0:
      f.write(f"yes,has_value,{i}_y\n")
    else:
      f.write(f"no,has_value,{i}_n\n")
  f.close()

main()
