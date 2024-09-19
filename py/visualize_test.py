#!/usr/bin/python3
import unittest

import numpy as np
from matplotlib import pyplot as plt

import hirm_io
import visualize


class testVisualize(unittest.TestCase):

  def test_figure_to_html(self):
    fig, ax = plt.subplots()
    x = np.arange(0.0, 2.0, 0.1)
    ax.plot(x, x * x)
    html = visualize.figure_to_html(fig)
    self.assertEqual(html[:9], '<img src=')

  def test_make_unary_matrix(self):
    observations = []
    observations.append(hirm_io.Observation(relation="R1", value="0", items=["A"]))
    observations.append(hirm_io.Observation(relation="R1", value="1", items=["B"]))
    observations.append(hirm_io.Observation(relation="R2", value="0", items=["A"]))
    observations.append(hirm_io.Observation(relation="R2", value="1", items=["B"]))
    m = visualize.make_unary_matrix(observations, ["A", "B"], ["R1", "R2"])
    self.assertEqual(m.shape, (2, 2))

  def test_make_binary_matrix(self):
    observations = []
    observations.append(hirm_io.Observation(relation="R1", value="0", items=["A", "A"]))
    observations.append(hirm_io.Observation(relation="R1", value="1", items=["B", "A"]))
    observations.append(hirm_io.Observation(relation="R1", value="0", items=["A", "B"]))
    observations.append(hirm_io.Observation(relation="R1", value="1", items=["B", "B"]))
    m = visualize.make_binary_matrix(observations, ["A", "B"])
    self.assertEqual(m.shape, (2, 2))

  def test_get_all_entities(self):
    c = hirm_io.Cluster(
        cluster_id="1", relations=["R1"],
        domain_clusters=[
            hirm_io.DomainCluster(cluster_id="0", domain="animals",
                                  entities=["dog", "cat", "elephant"]),
            hirm_io.DomainCluster(cluster_id="1", domain="animals",
                                  entities=["penguin"]),
            hirm_io.DomainCluster(cluster_id="3", domain="animals",
                                  entities=["mongoose", "eel", "human"])
        ])
    self.assertEqual(
        ["cat", "dog", "elephant", "penguin", "eel", "human", "mongoose"],
        visualize.get_all_entities(c))


if __name__ == '__main__':
  unittest.main()
