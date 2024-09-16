
import argparse

def main():
  parser = argparse.ArgumentParser(
      description="Generate a matrix plot for a binary relation")
  parser.add_argument(
      "observations", type=str,
      help="Path to file containing observations")
  parser.add_argument(
      "
