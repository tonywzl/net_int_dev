[global]
type = global
support_bio = 1
support_cds = 0
support_crc = 0
support_crcc = 0
support_dck = 0
support_drc = 0
support_drw = 1
support_drwc = 0
support_mrw = 1
support_mrwc = 1
support_bwc = 1
support_bwcc = 1
support_twc = 0
support_twcc = 0
support_rcc = 0
support_rio = 1
support_sac = 1
support_sds = 1
support_smc = 1
support_wcc = 1
support_shm = 0
support_dxt = 1
support_dxac = 1
support_dxpc = 1
support_dxa = 1
support_dxp = 1
support_reptc = 1
support_repsc = 1
support_reps = 1
support_rept = 1

[itest_tp_name]
type = tp
num_workers = 16

[sds_pp_name]
type = pp
pool_size = 512
page_size = 8

[rc_pp_name]
type = pp
pool_size = 16
page_size = 64

[itest_bwc_uuid]
type = bwc
cache_device = /wc_device
cache_size = 4096
rw_sync = 0
tp_name = itest_tp_name
flush_policy = bfp1
coalesce_ratio = 0.0


[itest_sds_name]
type = sds
pp_name = sds_pp_name
wc_uuid = itest_bwc_uuid

[itest_sds_namea]
type = sds
pp_name = sds_pp_name
wc_uuid = itest_bwc_uuid

[itest_mrw_name]
type = mrw

[itest_uuid_123456]
type = drw
exportname =/dev/sdb

[UUID1234567890]
type = sac
sync = 1
direct_io = 1
alignment = 4096
ds_name = itest_sds_name
dev_name = itest_mrw_name
dev_size = 10
volume_uuid = 3f3a8671-baaa-3db8-a0ff-023717636945

[test_dxta]
type = dxt
req_size = 32
ds_name = itest_sds_namea

[test_dxa]
type = dxa
ip = 10.16.158.203
peer_uuid = test_dxp
dxt_name = test_dxta

[test_reps]
type = reps
dxa_name = test_dxa
rept_name = test_rept
voluuid = 3f3a8671-baaa-3db8-a0ff-023717636945
vol_len = 10737418240
tp_name = itest_tp_name
