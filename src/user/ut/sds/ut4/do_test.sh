#!/bin/bash
for i in {1..1000}
do
	echo run test, $i time
	>/var/log/syslog
	./sds_unit
	echo $i time test done
        sleep 1
done

