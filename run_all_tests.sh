#!/bin/bash

set -e
#set -x

for file in `ls`; do
    if [ "test_" = "${file:0:5}" ];then
        echo "===========================RUNNING test $file==============================="
        ("./$file")
        echo "===========================PASSED  test $file==============================="
        echo ""
        echo ""
        echo ""
    fi
done

../run_logging_check_test.sh
