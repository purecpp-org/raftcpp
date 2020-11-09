  #!/usr/bin/env bash

set -x
# Cause the script to exit if a single command fails.
set -e

if [ -d "./build" ]; then
    rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 4
