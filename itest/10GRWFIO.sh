fio --directory=/tsfs/ --direct=1 --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=4k --rwmixread=50 --iodepth=1 --numjobs=50 --group_reporting --name=4ktestrw --size=120M