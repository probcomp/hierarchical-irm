# Copyright 2024
# Apache License, Version 2.0, refer to LICENSE.txt

import base64
import collections
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


def make_unary_matrix(obs, entities, relations):
  """Return a numpy array containing the observations for unary relations."""
  m = np.full((len(entities), len(relations)), np.nan)
  for ob in obs:
    val = ob.value
    ent_index = entities.index(ob.items[0])
    if ob.relation in relations:
      rel_index = relations.index(ob.relation)
      m[ent_index][rel_index] = val
  return m


def make_binary_matrix(obs, entities):
  """Return array containing the observations for a single binary relation."""
  m = np.full((len(entities), len(entities)), np.nan)
  for ob in obs:
    val = ob.value
    index1 = entities.index(ob.items[0])
    index2 = entities.index(ob.items[1])
    m[index1][index2] = val
  return m


def get_all_entities(cluster):
  """Return the entities in the order the cluster prefers."""
  all_entities = []
  domain = ""
  for dc in cluster.domain_clusters:
    if not domain:
      domain = dc.domain
    if domain != dc.domain:
      print("Error in unary_matrix: Found multiple domains in clusters")
      sys.exit(1)

    all_entities.extend(sorted(dc.entities))

  return all_entities


def add_redlines(ax, cluster, horizontal=True, vertical=False):
  """Add redlines between entity clusters."""
  n = 0
  for dc in cluster.domain_clusters:
    if n > 0:
      if horizontal:
        ax.axhline(n - 0.5, color='r', linewidth=2)
      if vertical:
        ax.axvline(n - 0.5, color='r', linewidth=2)

    n += len(dc.entities)


def unary_matrix(cluster, obs, clusters):
  """Plot a matrix visualization for the cluster of unary relations."""
  fontsize = 12

  relations = sorted(cluster.relations)
  num_columns = len(relations)
  longest_relation_length = max(len(r) for r in relations)

  all_entities = get_all_entities(cluster)
  num_rows = len(all_entities)
  longest_entity_length = max(len(e) for e in all_entities)

  width = (num_columns + longest_entity_length) * fontsize / 72.0 + 0.2 # Fontsize is in points, 1 pt = 1/72in
  height = (num_rows + longest_relation_length) * fontsize / 72.0 + 0.2
  fig, ax = plt.subplots(figsize=(width, height))

  ax.xaxis.tick_top()
  ax.set_xticks(np.arange(num_columns), labels=relations,
                rotation=90, fontsize=fontsize)
  ax.set_yticks(np.arange(num_rows), labels=all_entities, fontsize=fontsize)

  # Matrix of data
  m = make_unary_matrix(obs, all_entities, relations)
  cmap = copy.copy(plt.get_cmap())
  cmap.set_bad(color='white')
  ax.imshow(m, cmap=cmap)

  add_redlines(ax, cluster)

  fig.tight_layout()
  return fig


def binary_matrix(cluster, obs, clusters):
  """Plot a matrix visualization for a single binary relation."""
  fontsize = 12
  all_entities = get_all_entities(cluster)
  n = len(all_entities)
  longest_entity_length = max(len(e) for e in all_entities)
  width = (n + longest_entity_length) * fontsize / 72.0 + 0.2
  fig, ax = plt.subplots(figsize=(width, width))

  ax.xaxis.tick_top()
  ax.set_xticks(np.arange(n), labels=all_entities, rotation=90, fontsize=fontsize)
  ax.set_yticks(np.arange(n), labels=all_entities, fontsize=fontsize)

  m = make_binary_matrix(obs, all_entities)
  cmap = copy.copy(plt.get_cmap())
  cmap.set_bad(color='white')
  ax.imshow(m, cmap=cmap)

  add_redlines(ax, cluster, vertical=True)

  fig.tight_layout()
  return fig


def collate_observations(obs):
  """Separate observations into unary and binary."""
  unary_obs = []
  binary_obs = collections.defaultdict(list)
  for ob in obs:
    if len(ob.items) == 1:
      unary_obs.append(ob)

    if len(ob.items) == 2:
      binary_obs[ob.relation].append(ob)

  return unary_obs, binary_obs


def html_for_cluster(cluster, obs, clusters):
  """Return the html for visualizing a single cluster."""
  html = ''
  unary_obs, binary_obs = collate_observations(obs)
  if unary_obs:
    print(f"For irm #{cluster.cluster_id}, building unary matrix based on {len(unary_obs)} observations")
    fig = unary_matrix(cluster, unary_obs, clusters)
    html += figure_to_html(fig) + "\n"

  if binary_obs:
    for rel, rel_obs in binary_obs.items():
      print(f"For irm #{cluster.cluster_id}, building binary matrix for {rel} based on {len(rel_obs)} observations")
      html += f"<h2>{rel}</h2>\n"
      fig = binary_matrix(cluster, rel_obs, clusters)
      html += figure_to_html(fig) + "\n"
  return html


def make_plots(clusters, obs, output):
  """Write a matrix visualization for each cluster in clusters."""
  with open(output, 'w') as f:
    f.write("<html><body>\n\n")
    for cluster in clusters:
      f.write(f"<h1>IRM #{cluster.cluster_id}</h1>\n")
      f.write(html_for_cluster(cluster, obs, clusters) + "\n")
    f.write("</body></html>\n")
