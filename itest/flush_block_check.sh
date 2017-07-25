#!/bin/bash

ZFS="false"
if [ X"$1" == X"-zfs" ]; then
	ZFS="true"
fi

./runtest.sh bwctest $1

>flush_block_check.log
for((i=1; i<=100; i++)); do
	>/var/log/syslog
	echo -e "=================== Round $i ===================\n" >>flush_block_check.log

	echo "Generating data..." >>flush_block_check.log
	./write.sh 2M 8000 >>flush_block_check.log

	echo -e "\nChecking data integrity before recovery..." >>flush_block_check.log
	./read.sh 2M 8000 >>flush_block_check.log
	
	if [ "$ZFS" == "true" ]; then
		echo -e "\nChecking zfs status before recovery..." >>flush_block_check.log
		zpool status >>flush_block_check.log 2>&1
	fi
	
        let r=RANDOM%2
        if [ $r -eq 0 ]; then
                bwctest/nidmanager -r s -b ff
                echo -e "\nDoing fast flush..." >>flush_block_check.log
                sleep 20
        fi

	echo -e "\nKilling nidserver..." >>flush_block_check.log
	kill -9 `pidof nidserver`
	sleep 5

	echo -e "\nStarting nidserver..." >>flush_block_check.log
	bwctest/nidserver 
	sleep 10
	
	echo -e "\nChecking data integrity after recovery..." >>flush_block_check.log
	./read.sh 2M 8000 >>flush_block_check.log	# check data after recovery
	
	if [ "$ZFS" == "true" ]; then
		echo -e "\nChecking zfs status after recovery..." >>flush_block_check.log
		zpool status >>flush_block_check.log 2>&1
	fi
	
	echo -e "\nChecking flush block and flush seq..." >>flush_block_check.log
	end_line=`grep -n 'temp_test2' /var/log/syslog |tail -1 |cut -f 1 -d ':'` # the last line
	if [ -n "$end_line" ]; then
		temp_test1=`head -$end_line /var/log/syslog |grep 'temp_test1' |tail -1 |cut -f 6 -d ':'` # the flush block before stopped
		temp_test2=`head -$end_line /var/log/syslog |grep 'temp_test2' |tail -1 |cut -f 6 -d ':'` # the flush block after recovery
		temp_test3=`head -$end_line /var/log/syslog |grep 'temp_test3' |tail -1 |cut -f 6 -d ':'` # the flush seq before stopped
		temp_test4=`head -$end_line /var/log/syslog |grep 'temp_test4' |tail -1 |cut -f 6 -d ':'` # the flush seq after recovery
		if [ -n "$temp_test1" -a -n "$temp_test2" -a "$temp_test1" -lt "$temp_test2" -o \
			-n "$temp_test3" -a -n "$temp_test4" -a "$temp_test3" -lt "$temp_test4" ]; then
			echo "Warning: flush block before:$temp_test1, flush block after:$temp_test2, flush seq before:$temp_test3, flush seq after:$temp_test4" >>flush_block_check.log
		else
			echo "Success: flush block before:$temp_test1, flush block after:$temp_test2, flush seq before:$temp_test3, flush seq after:$temp_test4" >>flush_block_check.log	
		fi
	else
		end_line=`grep -n 'temp_test4' /var/log/syslog |tail -1 |cut -f 1 -d ':'` # the last line
		if [ -z "$end_line" ]; then
			echo "Warning: temp_test1-4 logging is not turned on" >>flush_block_check.log
			continue
		fi	
		temp_test1=`head -$end_line /var/log/syslog |grep 'temp_test1' |tail -1 |cut -f 6 -d ':'` # the flush block before stopped
		temp_test3=`head -$end_line /var/log/syslog |grep 'temp_test3' |tail -1 |cut -f 6 -d ':'` # the flush seq before stopped
		temp_test4=`head -$end_line /var/log/syslog |grep 'temp_test4' |tail -1 |cut -f 6 -d ':'` # the flush seq after recovery
		if [ -n "$temp_test3" -a -n "$temp_test4" -a "$temp_test3" -lt "$temp_test4" ]; then
			echo "Warning: nothing recovered, flush block before:$temp_test1, flush seq before:$temp_test3, flush seq after:$temp_test4" >>flush_block_check.log
		else
			echo "Success: nothing recovered, flush block before:$temp_test1, flush seq before:$temp_test3, flush seq after:$temp_test4" >>flush_block_check.log
		fi
	fi

	echo -e "\n\n" >>flush_block_check.log
done

if [ "$ZFS" == "true" ]; then
	zpool destroy -f usx
fi 
