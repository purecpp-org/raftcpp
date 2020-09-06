  #!/usr/bin/env bash

CURR_DIR=$(cd `dirname $0`; pwd)

clang-format -i $CURR_DIR/../src/*/*
clang-format -i $CURR_DIR/../tests/*

echo "============ Code formatted! ============="
