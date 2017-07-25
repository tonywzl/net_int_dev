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

echo "Success."
