#!/bin/bash
if [ $# -lt 1 ];then
	echo "Please input test name(Same as folder name). eg."
	echo "	./runtest.sh biotest";
	exit -1;
fi

cd ./$1
if [ $? -ne 0 ];then
	echo "Not existed test name"
	exit -1;
fi
echo "Begin clean ENV ..."

echo "Try umount /tsfs"
umount /tsfs
rm -r -f /tsfs

./nidmanager -r s -S
./nidmanager -r a -S

dd if=/dev/zero of=/wc_device bs=1M count=4
dd if=/dev/zero of=/rc_device bs=1M count=4
yes | mkfs.ext3 /wc_device; yes | mkfs.ext4 /wc_device
yes | mkfs.ext3 /rc_device; yes | mkfs.ext4 /rc_device

echo "Begin rm old nid device .."
rm -f /dev/nid*;
mknod /dev/nid0 b 44 0;
mknod /dev/nid1 b 44 16;
mknod /dev/nid2 b 44 32;

echo "Check nid mod"
MFS="true";
DEDUP="false";
ret=$(lsmod | grep "nid");
if [[ "$ret" == "" ]]; then
	echo "Insert nid.ko ..."
	echo ""
	insmod nid.ko
fi

if [ $# -eq 2 ];then
  if [[ $2 == "-k" ]];then 
	echo "Removing nid.ko ..."
	rmmod nid.ko;
	echo "Insert nid.ko ..."
	insmod nid.ko;
	if [ $? -ne 0 ];then
        	echo "Insert nid.ko error, please make sure this OS can support nid moudlue.";
        	exit -1;
	fi
  elif [[ $2 == "-r" ]]; then
	echo "Recover to default nid ENV ..."
  	cp /etc/ilio/nidserver.conf.bak /etc/ilio/nidserver.conf
	cp /etc/ilio/nidagent.conf.bak /etc/ilio/nidagent.conf
	nidserver
	nidagent
	nidmanager -r s -s get
	nidmanager -r a -s get
	echo "NID Recover successfully.";
	exit 0
  elif [[ $2 == "-b" ]]; then
	echo "Clean buffer device ..."
	rm /bufdevice
	touch /bufdevice
	exit 0
  elif [[ $2 == "-nfs" ]]; then
	MFS="false";
  elif [[ $2 == "-s" ]]; then
	./nidmanager -r s -S
	./nidmanager -r a -S
	exit 0
  elif [[ $2 == "-dedup" ]]; then
	DEDUP="true";
  fi

fi

cp nidserver.conf /etc/ilio/nidserver.conf

echo "Start nidserver ..."
./nidserver; 
sleep 10;
echo "Check nidserver state ..."
trycnt0=20

while [[ $trycnt0 > 0 ]]
do
	sleep 2;
	ret=$(./nidmanager -r s -s get | grep "rc:-");
        if [[ "$ret" == "" && $trycnt0 == 1 ]]; then
                ./nidmanager -r s -s get;
                echo "NID server state faild.";
                exit -1;
        elif [[ "$ret" != "" ]]; then
                echo "Try time left $trycnt0 : $ret"
                trycnt0=$[$trycnt0 - 1];
        else
                echo "Verify nidserver state working"
                break;
        fi
done

cp nidagent.conf /etc/ilio/nidagent.conf

echo ""
echo "Start nidagent ..."
./nidagent; 
trycnt1=100

echo ""
echo "Verify nidagent state ..."
while [[ $trycnt1 > 0 ]]
do
	sleep 10;

	ret=$(./nidmanager -r a -s get | grep "state:working");
	if [[ "$ret" == "" && $trycnt1 == 1 ]]; then
		./nidmanager -r a -s get;
		echo "NID agent state faild.";
		exit -1;
	elif [[ "$ret" == "" ]]; then
		echo "Try time left $trycnt1"
		trycnt1=$[$trycnt1 - 1];
	else
		echo "Verify nidagent state working"
		break;
	fi

done

echo ""
echo "Verify nidserver state ..."

trycnt2=10
while [[ $trycnt2 > 0 ]]
do
	sleep 2;
	ret=$(./nidmanager -r s -s get | grep "state:active");
	if [[ "$ret" == "" && $trycnt2 == 1 ]]; then
		./nidmanager -r s -s get
        	echo "NID server state error.";
		exit -1;
	elif [[ "$ret" == "" ]]; then
                trycnt2=$[$trycnt2 - 1];
        else
		echo "Verify nidserver state ok."
                break;
	fi
done

if [[ $MFS == "false" ]]; then
        exit 0;
fi


echo "Make ext3 FS on nid1"
if [[ $DEDUP == "false" ]]; then
	mkfs.ext3 /dev/nid1
	mkdir /tsfs
	mount /dev/nid1 /tsfs
else
	/opt/milio/bin/mke2fs -N 100000 -b 4096 -d -j -J  size=400 /dev/nid1
	mkdir /tsfs
	mount  -t dedup -o rw,noblocktable,noatime,nodiratime,timeout=180000,dedupzeros,thin_reconstruct,data=ordered,commit=30,errors=remount-ro /dev/nid1 /tsfs
fi

echo "================= dd into tsfs =================="
dd if=/dev/urandom of=/tsfs/tsfile bs=4K count=40000

# Make sure all data fllushed to device
sync
echo 3 > /proc/sys/vm/drop_caches

echo "================= Umounting tsfs =================="
umount /tsfs
rm -r -f /tsfs

echo "================= Stopping nidserver =================="
./nidmanager -r s -S

echo "================= Staring nidserver =================="
echo "Start nidserver ..."
./nidserver;
sleep 10;
echo "Check nidserver state ..."
trycnt0=20

while [[ $trycnt0 > 0 ]]
do
        sleep 2;
        ret=$(./nidmanager -r s -s get | grep "rc:-");
        if [[ "$ret" == "" && $trycnt0 == 1 ]]; then
                ./nidmanager -r s -s get;
                echo "NID server state faild.";
                exit -1;
        elif [[ "$ret" != "" ]]; then
                echo "Try time left $trycnt0 : $ret"
                trycnt0=$[$trycnt0 - 1];
        else
                echo "Verify nidserver state working"
                break;
        fi
done

trycnt1=100
echo ""
echo "Verify nidagent state ..."
while [[ $trycnt1 > 0 ]]
do
        sleep 10;

        ret=$(./nidmanager -r a -s get | grep "state:working");
        if [[ "$ret" == "" && $trycnt1 == 1 ]]; then
                ./nidmanager -r a -s get;
                echo "NID agent state faild.";
                exit -1;
        elif [[ "$ret" == "" ]]; then
                echo "Try time left $trycnt1"
                trycnt1=$[$trycnt1 - 1];
        else
                echo "Verify nidagent state working"
                break;
        fi

done

echo ""
echo "Verify nidserver state ..."

trycnt2=10
while [[ $trycnt2 > 0 ]]
do
        sleep 2;
        ret=$(./nidmanager -r s -s get | grep "state:active");
        if [[ "$ret" == "" && $trycnt2 == 1 ]]; then
                ./nidmanager -r s -s get
                echo "NID server state error.";
                exit -1;
        elif [[ "$ret" == "" ]]; then
                trycnt2=$[$trycnt2 - 1];
        else
                echo "Verify nidserver state ok."
                break;
        fi
done

echo "================= Mounting tsfs =================="
mkdir /tsfs
mount /dev/nid1 /tsfs

echo "ITest for $1 successfully."

exit 0
