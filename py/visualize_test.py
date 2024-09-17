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

  def test_make_numpy_matrix(self):
    observations = []
    observations.append(hirm_io.Observation(relation="R1", value="0", items=["A"]))
    observations.append(hirm_io.Observation(relation="R1", value="1", items=["B"]))
    observations.append(hirm_io.Observation(relation="R2", value="0", items=["A"]))
    observations.append(hirm_io.Observation(relation="R2", value="1", items=["B"]))
    m = visualize.make_numpy_matrix(observations, ["A", "B"], ["R1", "R2"])
    self.assertEqual(m.shape, (2, 2))

  def test_normalize_matrix(self):
    m = np.identity(3)
    np.testing.assert_equal(m, visualize.normalize_matrix(m))
    np.testing.assert_equal(m, visualize.normalize_matrix(3.0*m))
    np.testing.assert_equal([0, 0.25, 0.5, 0.75, 1.0],
                            visualize.normalize_matrix(np.arange(3, 8)))


if __name__ == '__main__':
  unittest.main()
