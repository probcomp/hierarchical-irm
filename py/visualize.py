# Copyright 2024
# Apache License, Version 2.0, refer to LICENSE.txt

import base64
import copy
import io
import sys
import numpy as np
from matplotlib import pyplot as plt


def figure_to_html(fig) -> str:
  """Return html for a matplotlib figure."""
  memfile = io.BytesIO()
  fig.savefig(memfile, format='png')
  encoded = base64.b64encode(memfile.getvalue()).decode('utf-8')
  html = '<img src=\'data:image/png;base64,{}\'>\n'.format(encoded)
  plt.close(fig)
  return html


def make_numpy_matrix(obs, entities, relations):
  """Return a numpy array containing the observations."""
  m = np.empty(shape=(len(entities), len(relations)))
  for ob in obs:
    val = ob.value
    ent_index = entities.index(ob.items[0])
    if ob.relation in relations:
      rel_index = relations.index(ob.relation)
      m[ent_index][rel_index] = val
  return m


def normalize_matrix(m):
  """Linearly map matrix values to be in [0, 1]."""
  return (m - np.min(m)) / np.ptp(m)


def unary_matrix(cluster, obs, clusters):
  """Plot a matrix visualization for the cluster of unary relations."""
  fontsize = 12

  relations = sorted(cluster.relations)
  num_columns = len(relations)
  longest_relation_length = max(len(r) for r in relations)

  all_entities = []
  domain = ""
  for dc in cluster.domain_clusters:
    if not domain:
      domain = dc.domain
    if domain != dc.domain:
      print("Error in unary_matrix: Found multiple domains in clusters")
      sys.exit(1)

    all_entities.extend(sorted(dc.entities))

  num_rows = len(all_entities)
  longest_entity_length = max(len(e) for e in all_entities)

  width = (num_columns + longest_entity_length) * fontsize / 72.0 + 0.2 # Fontsize is in points, 1 pt = 1/72in
  height = (num_rows + longest_relation_length) * fontsize / 72.0 + 0.2
  fig, ax = plt.subplots(figsize=(width,height))

  ax.xaxis.tick_top()
  ax.set_xticks(np.arange(num_columns), labels=relations,
                rotation=90, fontsize=fontsize)
  ax.set_yticks(np.arange(num_rows), labels=all_entities, fontsize=fontsize)

  # Matrix of data
  m = make_numpy_matrix(obs, all_entities, relations)
  cmap = copy.copy(plt.get_cmap('Greys'))
  cmap.set_bad(color='gray')
  ax.imshow(normalize_matrix(m))

  # Red lines between rows (entities)
  n = 0
  for dc in cluster.domain_clusters:
    if n > 0:
      ax.axhline(n - 0.5, color='r', linewidth=2)
    n += len(dc.entities)

  fig.tight_layout()
  return fig


def plot_unary_matrices(clusters, obs, output):
  """Write a matrix visualization for each cluster in clusters."""
  with open(output, 'w') as f:
    f.write("<html><body>\n\n")
    for cluster in clusters:
      f.write("<h1>IRM #" + cluster.cluster_id + "\n")
      fig = unary_matrix(cluster, obs, clusters)
      f.write(figure_to_html(fig) + "\n\n")
    f.write("</body></html>\n")
