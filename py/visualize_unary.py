#!/usr/bin/python3

import argparse
import hirm_io
import visualize

def main():
  parser = argparse.ArgumentParser(
      description="Generate matrix plots for a collection of unary relations")
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
  visualize.plot_unary_matrices(clusters, obs, args.output)


if __name__ == "__main__":
  main()
