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
    m = visualize.make_binary_matrix(observations, ["A", "B"], ["A", "B"])
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
    all_entities, big_dividers, small_dividers = visualize.get_all_entities(c)
    self.assertEqual(
        ["cat", "dog", "elephant", "penguin", "eel", "human", "mongoose"],
        all_entities)
    self.assertEqual(big_dividers, [])
    self.assertEqual(small_dividers, [3, 4])

  def test_get_all_entities_multidomain(self):
    c = hirm_io.Cluster(
        cluster_id="1", relations=["R1"],
        domain_clusters=[
            hirm_io.DomainCluster(cluster_id="0", domain="plants",
                                  entities=["oak", "sunflower", "carrot"]),
            hirm_io.DomainCluster(cluster_id="0", domain="animals",
                                  entities=["dog", "cat", "elephant"]),
            hirm_io.DomainCluster(cluster_id="1", domain="animals",
                                  entities=["penguin"]),
            hirm_io.DomainCluster(cluster_id="1", domain="plants",
                                  entities=["ash", "coconut", "fern"]),
            hirm_io.DomainCluster(cluster_id="3", domain="animals",
                                  entities=["mongoose", "eel", "human"]),
            hirm_io.DomainCluster(cluster_id="3", domain="plants",
                                  entities=["fig", "lilac", "potato"])
        ])
    all_entities, big_dividers, small_dividers = visualize.get_all_entities(c)
    self.assertEqual(
        ["cat", "dog", "elephant", "penguin", "eel", "human", "mongoose",
         "carrot", "oak", "sunflower", "ash", "coconut", "fern", "fig", "lilac",
         "potato"],
        all_entities)
    self.assertEqual(big_dividers, [7])
    self.assertEqual(small_dividers, [3, 4, 10, 13])

  def test_get_all_entities_for_domain(self):
    c = hirm_io.Cluster(
        cluster_id="1", relations=["R1"],
        domain_clusters=[
            hirm_io.DomainCluster(cluster_id="0", domain="plants",
                                  entities=["oak", "sunflower", "carrot"]),
            hirm_io.DomainCluster(cluster_id="0", domain="animals",
                                  entities=["dog", "cat", "elephant"]),
            hirm_io.DomainCluster(cluster_id="1", domain="animals",
                                  entities=["penguin"]),
            hirm_io.DomainCluster(cluster_id="1", domain="plants",
                                  entities=["ash", "coconut", "fern"]),
            hirm_io.DomainCluster(cluster_id="3", domain="animals",
                                  entities=["mongoose", "eel", "human"]),
            hirm_io.DomainCluster(cluster_id="3", domain="plants",
                                  entities=["fig", "lilac", "potato"])
        ])
    all_entities, dividers = visualize.get_all_entities_for_domain(c, "animals")
    self.assertEqual(
        ["cat", "dog", "elephant", "penguin", "eel", "human", "mongoose"],
        all_entities)
    self.assertEqual(dividers, [3, 4])

  def test_collate_observations(self):
    obs = [hirm_io.Observation(relation="R1", value="0", items=["A"]),
           hirm_io.Observation(relation="R1", value="1", items=["B"]),
           hirm_io.Observation(relation="R2", value="1", items=["A", "B"]),
           hirm_io.Observation(relation="R2", value="1", items=["B", "C"]),
           hirm_io.Observation(relation="R2", value="1", items=["B", "D"]),
           hirm_io.Observation(relation="R3", value="1", items=["B", "E", "F"]),
           hirm_io.Observation(relation="R3", value="1", items=["B", "G", "H"]),
           hirm_io.Observation(relation="R3", value="1", items=["B", "A", "B"])]
    unary_obs, binary_obs = visualize.collate_observations(obs)
    self.assertEqual(len(unary_obs), 2)
    self.assertEqual(len(binary_obs.keys()), 1)
    self.assertEqual(len(binary_obs["R2"]), 3)


if __name__ == '__main__':
  unittest.main()
