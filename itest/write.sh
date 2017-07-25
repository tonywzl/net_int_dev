#!/bin/bash

TEST_DATA_FILE=/tmp1/testdata$1$2
TEST_NID_FILE=/tsfs/testdata$1$2
BS=$1
COUNT=$2

sync
echo 3 > /proc/sys/vm/drop_caches

if [ ! -e $TEST_DATA_FILE ]; then
        echo "Make test data ..."
        touch $TEST_DATA_FILE
        dd if=/dev/urandom of=$TEST_DATA_FILE bs=$BS count=$COUNT oflag=sync

fi

echo ""
echo "Do dd write on tsfs ..."
time dd if=$TEST_DATA_FILE of=$TEST_NID_FILE bs=$BS count=$COUNT iflag=sync oflag=sync

echo "Success."
