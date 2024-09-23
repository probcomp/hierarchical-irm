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


def make_binary_matrix(obs, domain1_entities, domain2_entities):
  """Return array containing the observations for a single binary relation."""
  m = np.full((len(domain1_entities), len(domain2_entities)), np.nan)
  for ob in obs:
    val = ob.value
    index1 = domain1_entities.index(ob.items[0])
    index2 = domain2_entities.index(ob.items[1])
    m[index1][index2] = val
  return m


def get_all_entities(cluster):
  """Return the entities in the order the cluster prefers."""
  entities_by_domain = collections.defaultdict(list)
  dividers_by_domain = collections.defaultdict(list)
  for dc in cluster.domain_clusters:
    entities_by_domain[dc.domain].extend(sorted(dc.entities))
    dividers_by_domain[dc.domain].append(len(dc.entities))

  all_entities = []
  big_dividers = []
  small_dividers = []
  n = 0
  for domain in sorted(entities_by_domain.keys()):
    ent_list = entities_by_domain[domain]
    if n > 0:
      big_dividers.append(n)
    m = 0
    for sd in dividers_by_domain[domain]:
      m += sd
      small_dividers.append(n + m)
    small_dividers.pop()
    all_entities.extend(ent_list)
    n += len(ent_list)

  return all_entities, big_dividers, small_dividers


def get_all_entities_for_domain(cluster, domain):
  """Return the entities in the domain in the order the cluster likes."""
  all_entities = []
  dividers = []
  n = 0
  for dc in cluster.domain_clusters:
    if domain == dc.domain:
      all_entities.extend(sorted(dc.entities))
      if n > 0:
        dividers.append(n)
      n += len(dc.entities)

  return all_entities, dividers


def unary_matrix_plot(schema, cluster, obs, clusters):
  """Plot a matrix visualization for the cluster of unary relations."""
  fontsize = 12

  relations = sorted(cluster.relations)
  num_columns = len(relations)
  longest_relation_length = max(len(r) for r in relations)

  all_entities, big_dividers, small_dividers = get_all_entities(cluster)
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

  for n in big_dividers:
    ax.axhline(n - 0.5, color='black', linewidth=4)
  for n in small_dividers:
    ax.axhline(n - 0.5, color='r', linewidth=2)


  fig.tight_layout()
  return fig


def binary_matrix_plot(schema, cluster, obs, clusters):
  """Plot a matrix visualization for a single binary relation."""
  fontsize = 12
  relname = obs[0].relation

  if relname in schema:
    domain1 = schema[relname].domains[0]
    domain2 = schema[relname].domains[1]
  else:
    # No schema, assume only one domain.
    domain1 = cluster.domain_clusters[0].domain
    domain2 = cluster.domain_clusters[0].domain

  domain1_entities, domain1_dividers = get_all_entities_for_domain(
      cluster, domain1)
  domain2_entities, domain2_dividers = get_all_entities_for_domain(
      cluster, domain2)

  n1 = len(domain1_entities)
  n2 = len(domain2_entities)
  longest_domain1_entity_length = max(len(e) for e in domain1_entities)
  longest_domain2_entity_length = max(len(e) for e in domain2_entities)
  width = (n1 + longest_domain2_entity_length) * fontsize / 72.0 + 0.2
  height = (n2 + longest_domain1_entity_length) * fontsize / 72.0 + 0.2
  fig, ax = plt.subplots(figsize=(width, height))

  ax.xaxis.tick_top()
  ax.set_xticks(np.arange(n1), labels=all_domain1_entities, rotation=90, fontsize=fontsize)
  ax.set_yticks(np.arange(n2), labels=all_domain2_entities, fontsize=fontsize)

  m = make_binary_matrix(obs, domain1_entities, domain2_entities)
  cmap = copy.copy(plt.get_cmap())
  cmap.set_bad(color='white')
  ax.imshow(m, cmap=cmap)

  for n in domain1_dividers:
    ax.axhline(n - 0.5, color='r', linewidth=2)
  for n in domain2_dividers:
    ax.axvline(n - 0.5, color='r', linewidth=2)

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


def html_for_cluster(cluster, schema, obs, clusters):
  """Return the html for visualizing a single cluster."""
  html = ''
  unary_obs, binary_obs = collate_observations(obs)
  if unary_obs:
    print(f"For irm #{cluster.cluster_id}, building unary matrix based on {len(unary_obs)} observations")
    fig = unary_matrix_plot(schema, cluster, unary_obs, clusters)
    html += figure_to_html(fig) + "\n"

  if binary_obs:
    for rel, rel_obs in binary_obs.items():
      print(f"For irm #{cluster.cluster_id}, building binary matrix for {rel} based on {len(rel_obs)} observations")
      html += f"<h2>{rel}</h2>\n"
      fig = binary_matrix_plot(schema, cluster, rel_obs, clusters)
      html += figure_to_html(fig) + "\n"
  return html


def make_plots(schema, obs, clusters, output):
  """Write a matrix visualization for each cluster in clusters."""
  with open(output, 'w') as f:
    f.write("<html><body>\n\n")
    for cluster in clusters:
      f.write(f"<h1>IRM #{cluster.cluster_id}</h1>\n")
      f.write(html_for_cluster(cluster, schema, obs, clusters) + "\n")
    f.write("</body></html>\n")
