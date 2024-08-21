# Generates a 4 column dataset for one data type.
import numpy as np

NUM_SAMPLES_1 = 33
NUM_SAMPLES_2 = 50
NUM_SAMPLES = 100


def write_out_dataset(filename, dataset):
  f = open(filename, 'w')
  for i in range(NUM_SAMPLES):
    f.write(f"{dataset[i, 0]},col1,{i}\n")
    f.write(f"{dataset[i, 1]},col2,{i}\n")
    f.write(f"{dataset[i, 2]},col3,{i}\n")
    f.write(f"{dataset[i, 3]},col4,{i}\n")
  f.close()

def main():
  rng = np.random.default_rng(1762)
  # Generate real data.

  # Draw from a well separated mixture of 3 2-D normals for the first view, and 2 2-D normals for the second view.
  e = 0.5 * np.eye(2)
  cluster1 = rng.multivariate_normal([-5., -5.], e, NUM_SAMPLES_1)
  cluster2 = rng.multivariate_normal([0., 0.], e, NUM_SAMPLES_1)
  cluster3 = rng.multivariate_normal([5., 5.], e, NUM_SAMPLES_1 + 1)
  view1 = np.concatenate([cluster1, cluster2, cluster3], axis=0)
  # Make sure that the first view doesn't non-trivially correlate with the second view.
  rng.shuffle(view1)

  # Use a obviously different correlation here.
  cov = [[1., 1. / 4.], [1. / 4., 1. / 2.]]
  cluster1 = rng.multivariate_normal([5., -5.], cov, NUM_SAMPLES_2)
  cluster2 = rng.multivariate_normal([1., 1.], cov, NUM_SAMPLES_2)
  view2 = np.concatenate([cluster1, cluster2], axis=0)

  dataset = np.concatenate([view1, view2], axis=1)
  write_out_dataset('test_crosscat_normal.obs', dataset)

  # Generate boolean data.
  # For the first view, the columns are duplicated. For the second view, one is the opposite of the other.
  data = rng.integers(0, 2, size=NUM_SAMPLES)
  view1 = np.stack([data, data], axis=1)

  data = rng.integers(0, 2, size=NUM_SAMPLES)
  view2 = np.stack([data, 1 - data], axis=1)

  dataset = np.concatenate([view1, view2], axis=1)
  write_out_dataset('test_crosscat_bernoulli.obs', dataset)

  # Generate string data.
  string1 = ['aba', 'baa', 'abb']
  string2 = ['1', '2', '3']

  string3 = ['cccd', 'ddcccc']
  string4 = ['first', 'second']

  column1 = np.random.choice(string1, NUM_SAMPLES, p=[1./3, 1./3, 1./3])
  column2 = np.where(np.equal(column1, 'aba'), '1',
                     np.where(np.equal(column1, 'baa'), '2', '3'))
  view1 = np.stack([column1, column2], axis=1)

  column3 = np.random.choice(string3, NUM_SAMPLES, p=[0.5, 0.5])
  column4 = np.where(np.equal(column1, 'cccd'), 'first', 'second')
  view2 = np.stack([column3, column4], axis=1)
  dataset = np.concatenate([view1, view2], axis=1)
  write_out_dataset('test_crosscat_string.obs', dataset)

  # Generate categorical data.
  category1 = ['a', 'b', 'c', 'd']
  category2 = ['first', 'second']

  category3 = ['1', '2', '3', '4']
  category4 = ['odd', 'even']

  column1 = np.random.choice(category1, NUM_SAMPLES, p=[0.25, 0.25, 0.25, 0.25])
  column2 = np.where(np.equal(column1, 'a') | np.equal(column1, 'b'), 'first', 'second')
  view1 = np.stack([column1, column2], axis=1)

  column3 = np.random.choice(category3, NUM_SAMPLES, p=[0.25, 0.25, 0.25, 0.25])
  column4 = np.where(np.equal(column3, '1') | np.equal(column1, '3'), 'odd', 'even')
  view2 = np.stack([column3, column4], axis=1)
  dataset = np.concatenate([view1, view2], axis=1)
  write_out_dataset('test_crosscat_categorical.obs', dataset)

  
main()
