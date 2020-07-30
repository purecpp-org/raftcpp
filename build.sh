  #!/usr/bin/env bash

set -x
# Cause the script to exit if a single command fails.
set -e

mkdir build
cd build
cmake ..
make

