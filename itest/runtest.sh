#!/bin/bash
if [ $# -lt 1 ];then
	echo "Please input test name(Same as folder name). eg."
	echo "	./runtest.sh bwctest";
	exit -1;
fi

cd ./$1
if [ $? -ne 0 ];then
	echo "Not existed test name"
	exit -1;
fi
echo "Begin clean ENV ..."

./nidmanager -r s -S
./nidmanager -r a -S

is_bwctest=0
if [ $1 = "bwctest" ] || [ $1 = "bwctest/" ];then
        is_bwctest=1
fi

# get cache device info from conf file instead of a fixed one
cache_dev=$(grep cache_device nidserver.conf |awk '{print $3}');
if [[ "$cache_dev" != "" ]];then
	# only need do these in run test mode
	if [ $# -eq 1 ];then
		if [ $is_bwctest -eq 1 ];then
			./bwc_ver_cmp $cache_dev
		fi
		dd if=/dev/zero of=$cache_dev bs=1M count=100
		dd if=/dev/zero of=$cache_dev bs=1M count=100 seek=16284
	fi
fi

if [ -d "/tsfs" ]; then
	echo "Try umount /tsfs"
	umount /tsfs
	rm -r -f /tsfs
fi

echo "Begin rm old nid device .."
rm -f /dev/nid*
mknod /dev/nid0 b 44 0;
rm -f /dev/nid2;
mknod /dev/nid2 b 44 32;

echo "Check nid mod"
MFS="true";
DEDUP="false";
ZFS="false";
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
	if [[ "$cache_dev" != "" ]];then
		rm $cache_dev
		touch $cache_dev
	fi
	exit 0
  elif [[ $2 == "-nfs" ]]; then
	MFS="false";
  elif [[ $2 == "-s" ]]; then
	./nidmanager -r s -S
	./nidmanager -r a -S
	exit 0
  elif [[ $2 == "-dedup" ]]; then
	DEDUP="true";
  elif [[ $2 == "-zfs" ]]; then
	ZFS="true";
  fi

fi

>/var/log/syslog

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

nomgr=0
cp nidagent.conf /etc/ilio/nidagent.conf
if [ ! -f "/usr/local/bin/nidmanager" ]; then
	nomgr=1
	cp nidmanager /usr/local/bin/nidmanager
fi

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


echo "Make FS on nid2"
if [[ $DEDUP == "true" ]]; then
	/opt/milio/bin/mke2fs -N 100000 -b 4096 -d -j -J  size=400 /dev/nid2
	mkdir /tsfs
	mount  -t dedup -o rw,noblocktable,noatime,nodiratime,timeout=180000,dedupzeros,thin_reconstruct,data=ordered,commit=30,errors=remount-ro /dev/nid2 /tsfs
elif [[ $ZFS == "true" ]]; then
	mkdir /tsfs
	/sbin/modprobe zfs
	echo 1073741824 > /sys/module/zfs/parameters/zfs_arc_max
	zpool create usx -f -m /tsfs -o ashift=12 /dev/nid2
	zfs set dedup=on usx
	zfs set recordsize=4096 usx
else
	mkfs.ext3 /dev/nid2
	mkdir /tsfs
	mount /dev/nid2 /tsfs
fi

# Make sure all data fllushed to device
sync
echo 3 > /proc/sys/vm/drop_caches

# clean up nidmanager file created in test when there have no nidmanager in system
if [[ $nomgr -eq 1 ]];then
        rm /usr/local/bin/nidmanager
fi

echo "ITest for $1 successfully."
exit 0
