#!/usr/bin/python3
import unittest

import hirm_io

class TestHirmIo(unittest.TestCase):

  def test_load_observations(self):
    observations = hirm_io.load_observations("../cxx/assets/animals.unary.obs")
    self.assertEqual(len(observations), 4250)
    self.assertEqual(observations[0].relation, "black")
    self.assertEqual(observations[0].value, "0")
    self.assertEqual(observations[0].items, ["antelope"])

  def test_load_clusters(self):
    pass


if __name__ == '__main__':
  unittest.main()
