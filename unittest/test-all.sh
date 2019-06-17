#!/bin/sh -l

set -e

device_tag=$1

./unittest.bin${device_tag}

echo "All tests completed"
echo "We have none, yet ;-)"
