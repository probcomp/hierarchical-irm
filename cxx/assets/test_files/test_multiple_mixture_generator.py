# Generates observations/schema for a set of Gaussian mixture models. Each
# mixture has a single domain (corresponding to samples) and two relations:
# one equal to the sample value and one that is a mixture component label.
# We expect the HIRM to cluster the samples/labels from a given mixture model
# in the same view, and that within a view, samples from mixture components
# will cluster with their labels.
import numpy as np

# Set a lowest mean for each mixture model.
LOWEST_MIXTURE_COMPONENT_MEANS = {"a": -1., "b": 1.}

# Number of components in each mixture model, and the spacing of their means.
NUM_MIXTURE_COMPONENTS = 3
COMPONENT_SPACING = 2.
STDDEV = 0.5

# Number of samples per mixture component (each for labels/values).
NUM_SAMPLES = 100

def main():
  rng = np.random.default_rng(1762)
  f_obs = open('test-n-mixture-k-normals.obs', 'w')
  for label, lowest in LOWEST_MIXTURE_COMPONENT_MEANS.items():
    mean = lowest
    for i in range(NUM_MIXTURE_COMPONENTS):
      samples_val = rng.normal(mean, STDDEV, NUM_SAMPLES)
      samples_label = rng.normal(mean, STDDEV, NUM_SAMPLES)
      for j in range(NUM_SAMPLES):
        f_obs.write(f"{samples_val[j]},value_{label},{label}{i}_{samples_val[j]}\n")
        f_obs.write(f"{label}{i},label_{label},{label}{i}_{samples_label[j]}\n")
      mean += COMPONENT_SPACING
  f_obs.close()

  f_schema = open('test-n-mixture-k-normals.schema', 'w')
  for label in LOWEST_MIXTURE_COMPONENT_MEANS.keys():
    f_schema.write(f"value_{label} ~ normal({label})\n")
    components = ":".join(f"{label}{k}" for k in range(NUM_MIXTURE_COMPONENTS))
    f_schema.write(
      f"label_{label} ~ stringcat[strings=\"{components}\",delim=:]({label})\n")
  f_schema.close()

main()
