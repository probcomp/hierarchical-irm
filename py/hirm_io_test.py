#!/usr/bin/python3
import unittest

import hirm_io

class TestHirmIo(unittest.TestCase):

  def test_load_schema(self):
    relations = hirm_io.load_schema("../cxx/assets/animals.unary.schema")
    self.assertEqual(len(relations), 85)
    self.assertEqual(relations["black"].name, "black")
    self.assertEqual(relations["black"].distribution, "bernoulli")
    self.assertEqual(relations["black"].domains, ["animal"])

  def test_load_schema_with_comments(self):
    relations = hirm_io.load_schema("../cxx/assets/animals.unary.with_comments.schema")
    self.assertEqual(len(relations), 85)
    self.assertEqual(relations["black"].name, "black")
    self.assertEqual(relations["black"].distribution, "bernoulli")
    self.assertEqual(relations["black"].domains, ["animal"])
    self.assertEqual(relations["big"].name, "big")
    self.assertEqual(relations["big"].distribution, "bernoulli")
    self.assertEqual(relations["big"].domains, ["animal"])

  def test_load_schema_with_distribution_parameters(self):
    relations = hirm_io.load_schema("../cxx/assets/pclean_rents_clean_ternary.schema")
    self.assertEqual(len(relations), 3)
    self.assertEqual(relations["has_type"].name, "has_type")
    self.assertEqual(relations["has_type"].distribution, "stringcat")
    self.assertEqual(len(relations["has_type"].parameters), 1)
    print(relations["has_type"].parameters)
    self.assertEqual(relations["has_type"].parameters["strings"],
                     '"1br 2br 3br 4br studio"')
    self.assertEqual(relations["has_type"].domains, ["record", "County", "State"])

  def test_load_observations(self):
    observations = hirm_io.load_observations("../cxx/assets/animals.unary.obs")
    self.assertEqual(len(observations), 4250)
    self.assertEqual(observations[0].relation, "black")
    self.assertEqual(observations[0].value, "0")
    self.assertEqual(observations[0].items, ["antelope"])

  def test_load_clusters(self):
    clusters = hirm_io.load_clusters("../cxx/assets/animals.unary.hirm")
    self.assertEqual(len(clusters), 4)
    self.assertEqual(clusters[0].cluster_id, "0")
    self.assertEqual(clusters[1].cluster_id, "5")
    self.assertEqual(clusters[2].cluster_id, "7")
    self.assertEqual(clusters[3].cluster_id, "8")
    self.assertEqual(clusters[0].relations[0], "quadrapedal")
    self.assertEqual(len(clusters[0].domain_clusters), 3)
    self.assertEqual(clusters[0].domain_clusters[0].cluster_id, "0")
    self.assertEqual(clusters[0].domain_clusters[0].domain, "animal")
    self.assertEqual(clusters[0].domain_clusters[0].entities[0], "antelope")


if __name__ == "__main__":
  unittest.main()
