#!/bin/bash

# Fail on any error
set -e
# Echo the command being run
set -x

# Run integration test suite
bazel build tests:test_hirm_animals tests:test_irm_two_relations tests:test_misc
startt1=$(date +%s)
./bazel-bin/tests/test_hirm_animals
./bazel-bin/tests/test_irm_two_relations
./bazel-bin/tests/test_misc
endt1=$(date +%s)
bazel build -c opt :hirm pclean:pclean
./bazel-bin/hirm --mode=irm --iters=5 assets/animals.binary
startt2=$(date +%s)
./bazel-bin/hirm --seed=1 --iters=5 assets/nations.unary --samples=5
./bazel-bin/hirm --iters=5 --load=assets/animals.unary.1.hirm assets/animals.unary
endt2=$(date +%s)
startt3=$(date +%s)
./bazel-bin/pclean/pclean --schema=assets/flights.schema --obs=assets/flights_dirty.10.csv --iters=5 --heldout=assets/flights_dirty.last10.csv --samples=5 --output=/tmp/flights.output
./bazel-bin/pclean/pclean --schema=assets/hospitals.schema --obs=assets/hospital_dirty.10.csv --iters=5 --inference_iters=2 --only_final_emissions
./bazel-bin/pclean/pclean --schema=assets/rents.schema --obs=assets/rents_dirty.10.csv --iters=5 --record_class_is_clean --heldout=assets/rents_dirty.last10.csv
endt3=$(date +%s)
echo "Integration tests in /tests ran in $(($endt1-$startt1)) seconds"
echo "hirm integration tests ran in $(($endt2-$startt2)) seconds"
echo "pclean integration tests ran in $(($endt3-$startt3)) seconds"
