# Copyright 2024
# Apache License, Version 2.0, refer to LICENSE.txt

import collections

Observation = namedtuple("Observation", ["relation", "value", "items"])

Cluster = namedtuple("Cluster", ["cluster_id", "relations", "domain_clusters"])
Domain_Cluster = namedtuple("Domain_Cluster", ["cluster_id", "domain",
                                               "entities"])

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
  id_to_relations = {}

  with open(path, 'r') as f:
    # Read the relation clusters
    for line in f:
      fields = line.split()
      if len(fields) == 0:
        break
      id_to_relations[fields[0]] = fields[1:]

    # Read the IRM clusters
    for line in f:
      fields = line.split()


  return clusters
