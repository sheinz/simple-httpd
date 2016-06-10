#!/bin/bash

TEST_DATA_FILE=/tmp/test.data

echo 'Generating test data file'
dd if=/dev/urandom of=${TEST_DATA_FILE} bs=1M count=32

./test &
PID=$!

echo "Web server is running with PID=$PID"

if python3 post.py
then
    echo "Data uploaded"
else
    echo "Fail to upload data"
    exit 1
fi

kill ${PID} 2> /dev/null

PARAM_1=`cat param_1`
PARAM_2=`cat param_2`
PARAM_3=`cat param_3`

if [ "$PARAM_1" != "value_1" ]
then
    echo "Param value incorrect"
    exit 1
fi

if [ "$PARAM_2" != "value_2" ]
then
    echo "Param value incorrect"
    exit 1
fi

if [ "$PARAM_3" != "value_3" ]
then
    echo "Param value incorrect"
    exit 1
fi


shasum ${TEST_DATA_FILE} > ${TEST_DATA_FILE}.sha

if shasum firmware -c ${TEST_DATA_FILE}.sha | grep -q 'OK'
then
    echo 'Test OK'
    exit 0
else
    echo 'Test failed'
    exit 1
fi



