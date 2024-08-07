#!/bin/bash

# Fail on any error
set -e
# Echo the command being run
set -x

# Run integration test suite
bazel build :hirm pclean:pclean tests:test_hirm_animals tests:test_irm_two_relations tests:test_misc
./bazel-bin/tests/test_hirm_animals
./bazel-bin/tests/test_irm_two_relations
./bazel-bin/tests/test_misc
./bazel-bin/hirm --mode=irm --iters=5 assets/animals.binary
./bazel-bin/hirm --seed=1 --iters=5 assets/animals.unary
./bazel-bin/hirm --iters=5 --load=assets/animals.unary.1.hirm assets/animals.unary
./bazel-bin/pclean/pclean --schema=assets/flights.schema --obs=assets/flights_dirty.100.csv --iters=5
./bazel-bin/pclean/pclean --schema=assets/hospitals.schema --obs=assets/hospital_dirty.100.csv --iters=5
./bazel-bin/pclean/pclean --schema=assets/rents.schema --obs=assets/rents_dirty.100.csv --iters=5
