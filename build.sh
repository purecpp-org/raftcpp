  #!/usr/bin/env bash

set -x
# Cause the script to exit if a single command fails.
set -e

bazel --version
bazel build --jobs=4 //:all
