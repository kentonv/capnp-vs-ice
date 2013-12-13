#! /bin/bash

# Builds everything, sets up the latency faker, then runs the Cap'n Proto
# test followed by the Ice test.

set -euo pipefail

make -j

./fake-latency &
FPID=${!}

echo "*** Testing Cap'n Proto..."

./capnp-server &
PID=${!}

./capnp-client

kill $PID
wait $PID > /dev/null 2>&1 || true

echo "*** Cap'n Proto done."

echo "*** Testing ZeroC Ice..."

./ice-server &
PID=${!}

./ice-client

kill $PID
wait $PID > /dev/null 2>&1 || true

kill $FPID
wait $FPID > /dev/null 2>&1 || true

echo "*** Ice done."
