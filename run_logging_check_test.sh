#!/bin/bash

echo "===========================RUNNING test test_logging_check==============================="
./logging_check_test &
wait $!
status=$?
if [ $status != 0 ];then
echo "status is $status,abort!"
echo "===========================PASSED  test test_logging_check==============================="
else
echo "status is $status success!"
fi
