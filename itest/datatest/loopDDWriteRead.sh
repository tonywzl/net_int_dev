#!/bin/bash

>loop$1$2$3.log;

for ((i=0; i<$3; ++i))
do
	echo "" >> loop$1$2$3.log
	echo "===============$i============" >> loop$1$2$3.log
	./rwbk.sh $1 $2 &>>loop$1$2$3.log;
	ret=$(cat loop$1$2$3.log | grep ERROR);
        if [[ "$ret" != "" ]]; then
                echo "$ret";
		exit 1;
        fi

done


