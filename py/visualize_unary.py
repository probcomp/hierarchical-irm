#!/usr/bin/python3

import argparse
import hirm_io
import visualize

def main():
  parser = argparse.ArgumentParser(
      description="Generate matrix plots for a collection of unary relations")
  parser.add_argument(
      "observations", type=str, required=True,
      help="Path to file containing observations")
  parser.add_argument(
      "clusters", type=str, required=True,
      help="Path to file containing HIRM output cluster")
  parser.add_argument(
      "output", type=str, required=True,
      help="Prefix to all output files")

  args = parser.parse_args()

  obs = hirm_io.load_observations(args.observations)
  clusters = hirm_io.load_observations(args.clusters)

  visualize.plot_unary_matrices(clusters, obs, args.output)
