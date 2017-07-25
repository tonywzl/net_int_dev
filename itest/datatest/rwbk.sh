TEST_DATA_FILE=/tmp/testdatabk$1$2
TEST_DATA_FILE_OUT=/tmp/testdatabk_out$1$2
BS=$1
CNT=$2


sync
echo 3 > /proc/sys/vm/drop_caches

if [ ! -e $TEST_DATA_FILE ]; then
        echo "Make test data ..."
        touch $TEST_DATA_FILE
        dd if=/dev/urandom of=$TEST_DATA_FILE bs=$BS count=$CNT 

fi

echo ""
echo "Do dd write on device nid1 ..."
time dd if=$TEST_DATA_FILE of=/dev/nid1 bs=$BS count=$CNT iflag=sync oflag=sync

>$TEST_DATA_FILE_OUT

echo ""
echo "Do dd read on device nid1 ..."
time dd if=/dev/nid1 of=$TEST_DATA_FILE_OUT bs=$BS count=$CNT iflag=sync oflag=sync

echo "Do MD5 and check read write file if match or not ..."
DATA_MD5=$(md5sum $TEST_DATA_FILE)
DATA_OUT_MD5=$(md5sum $TEST_DATA_FILE_OUT)
DATA_MD5=(${DATA_MD5///})
DATA_OUT_MD5=(${DATA_OUT_MD5///})

if [ "$DATA_MD5" != "$DATA_OUT_MD5" ]; then
        echo "ERROR: Read file not match write file!!"
        exit -1
fi
sync
echo 3 > /proc/sys/vm/drop_caches

echo "Success."
