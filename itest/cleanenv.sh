#./bin/bash
cqlsh -f cleandb.cql


echo "create keyspace mstest with replication = {'class': 'SimpleStrategy', 'replication_factor':1};" | cqlsh

echo "use mstest; DROP TABLE ns4; DROP TABLE ns; DROP TABLE dedup;" | cqlsh

echo "use mstest;CREATE TABLE dedup (fp blob, pobj blob, PRIMARY KEY (fp)) WITH bloom_filter_fp_chance=0.010000 AND caching='{\"keys\":\"ALL\", \"rows_per_partition\":\"NONE\"}' AND comment='' AND dclocal_read_repair_chance=0.100000 AND gc_grace_seconds=864000 AND read_repair_chance=0.000000 AND default_time_to_live=0 AND speculative_retry='99.0PERCENTILE' AND memtable_flush_period_in_ms=0 AND compaction={'class': 'SizeTieredCompactionStrategy'} AND compression={'sstable_compression': 'LZ4Compressor'};" | cqlsh

echo "use mstest;CREATE TABLE ns ( off int, v2fp map<bigint, blob>, PRIMARY KEY (off)) WITH  bloom_filter_fp_chance=0.010000 AND  comment='' AND  dclocal_read_repair_chance=0.100000 AND caching='{\"keys\":\"ALL\", \"rows_per_partition\":\"NONE\"}' AND  gc_grace_seconds=864000 AND  read_repair_chance=0.000000 AND  default_time_to_live=0 AND  speculative_retry='99.0PERCENTILE' AND  memtable_flush_period_in_ms=0 AND  compaction={'class': 'SizeTieredCompactionStrategy'} AND  compression={'sstable_compression': 'LZ4Compressor'};" | cqlsh

echo "use mstest;CREATE TABLE ns4 (volid bigint,volindex bigint,offset int,fp blob,PRIMARY KEY ((volid, volindex), offset))WITH CLUSTERING ORDER BY (offset DESC);" | cqlsh

ceph osd pool delete mstestpool mstestpool --yes-i-really-really-mean-it
ceph osd pool create mstestpool 100

