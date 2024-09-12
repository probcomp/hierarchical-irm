# Copyright 2024
# Apache License, Version 2.0, refer to LICENSE.txt

import collections

Observation = namedtuple("Observation", ["relation", "value", "items"])

IRM_Cluster = namedtuple("IRM_Cluster", ["domain", "cluster_id", "entities"])

HIRM_Clusters = namedtuple("HIRM_Clusters", ["relation_clusters",
                                             "irm_clusters"])

def load_observations(path):
  """Load a dataset from path, and return it as an array of Observations."""
  data = []
  with open(path, 'r') as f:
        for line in f:
          fields = line.split(',')
          assert len(fields) >= 3
          data.append(Observation(relation=fields[1],
                                  value=fields[0],
                                  items=fields[2:]))
  return data

def load_clusters(path):
  """Load the hirm clusters output from path as an HIRM_Clusters."""
  rel_clusters = []
  irm_clusters = []
  with open(path, 'r') as f:
    # Read the relation clusters
    for line in f:
      fields = line.split()
      if len(fields) == 0:
        break
      rel_clusters.append(fields[1:])


  return HIRM_Clusters(relation_clusters=rel_clusters,
                       irm_clusters=irm_clusters)
