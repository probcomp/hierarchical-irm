#!/usr/bin/python3

# Generates an html file containing plots that visualize the clusters
# produced by an HIRM.  Currently it only supports datasets with a single
# domain, and only visualizes the unary and binary relations over that
# domain.

# Example usage:
#   ./make_plots.py --observations=../cxx/assets/animals.unary.obs --clusters=../cxx/assets/animals.unary.hirm --output=/tmp/vis.html

import argparse
import hirm_io
import visualize

def main():
  parser = argparse.ArgumentParser(
      description="Generate matrix plots for an HIRM's output")
  parser.add_argument(
      "--observations", required=True, type=str,
      help="Path to file containing observations")
  parser.add_argument(
      "--clusters", required=True, type=str,
      help="Path to file containing HIRM output cluster")
  parser.add_argument(
      "--output", required=True, type=str,
      help="Path to write HTML output to.")

  args = parser.parse_args()

  print(f"Loading observations from {args.observations} ...")
  obs = hirm_io.load_observations(args.observations)
  print(f"Loading clusters from {args.clusters} ...")
  clusters = hirm_io.load_clusters(args.clusters)

  print(f"Writing html to {args.output} ...")
  visualize.make_plots(clusters, obs, args.output)


if __name__ == "__main__":
  main()
