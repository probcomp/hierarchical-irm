# Copyright 2024
# Apache License, Version 2.0, refer to LICENSE.txt

import collections

Observation = collections.namedtuple(
    "Observation", ["relation", "value", "items"])

Cluster = collections.namedtuple(
    "Cluster", ["cluster_id", "relations", "domain_clusters"])
Domain_Cluster = collections.namedtuple(
    "Domain_Cluster", ["cluster_id", "domain", "entities"])

def load_observations(path):
  """Load a dataset from path, and return it as an array of Observations."""
  data = []
  with open(path, 'r') as f:
        for line in f:
          fields = line.rstrip().split(',')
          assert len(fields) >= 3
          data.append(Observation(relation=fields[1],
                                  value=fields[0],
                                  items=fields[2:]))
  return data

def load_clusters(path):
  """Load the hirm clusters output from path as an HIRM_Clusters."""
  id_to_relations = {}
  id_to_clusters = {}

  with open(path, 'r') as f:
    # Read the relation clusters
    for line in f:
      fields = line.rstrip().split()
      if not fields:
        break
      id_to_relations[fields[0]] = fields[1:]

    # Read the IRM clusters
    domain_clusters = []
    cluster_id = -1
    for line in f:
      fields = line.rstrip().split()
      if not fields:
        assert(cluster_id != -1)
        id_to_clusters[cluster_id] = domain_clusters
        domain_clusters = []
        cluster_id = -1
        continue

      if len(fields) == 1:
        parts = fields[0].split('=')
        assert len(parts) == 2
        cluster_id = parts[1]
        continue

      domain_clusters.append(
          Domain_Cluster(cluster_id=fields[1],
                         domain=fields[0],
                         entities=fields[2:]))

    id_to_clusters[cluster_id] = domain_clusters

  clusters = []
  for cluster_id, relations in id_to_relations.items():
    clusters.append(Cluster(cluster_id=cluster_id, relations=relations,
                            domain_clusters=id_to_clusters[cluster_id]))
  return clusters
