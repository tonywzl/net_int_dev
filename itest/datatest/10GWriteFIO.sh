fio -filename=/dev/nid1 -direct=1 -iodepth 1 -thread -rw=randrw -ioengine=psync -bs=1M -size=10G -numjobs=30 -runtime=1000 -group_reporting -name=mytest
#fio -filename=/dev/nid1 -direct=1 -iodepth 1 -thread -rw=randwrite -ioengine=psync -bs=1M -size=10G -numjobs=30 -runtime=1000 -group_reporting -name=mytest
#fio --directory=/mnt/ --direct=1 --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=4k --rwmixread=50 --iodepth=1 --numjobs=100 --group_reporting --name=4ktestrw --size=500M
