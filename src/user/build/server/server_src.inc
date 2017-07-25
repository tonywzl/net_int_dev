
CACHE_BASEDIR = $(BASEDIR)/cache
CONF_BASEDIR = $(BASEDIR)/conf
DEV_BASEDIR = $(BASEDIR)/dev
UMPK_BASEDIR = $(BASEDIR)/umpk
MM_BASEDIR = $(BASEDIR)/mm
TX_BASEDIR = $(BASEDIR)/tx
RW_BASEDIR = $(BASEDIR)/rw
REP_BASEDIR = $(BASEDIR)/rep

include $(BUILDDIR)/server/cache_src.inc
include $(BUILDDIR)/server/conf_src.inc
include $(BUILDDIR)/server/dev_src.inc
include $(BUILDDIR)/server/tx_src.inc
include $(BUILDDIR)/server/umpk_src.inc
include $(BUILDDIR)/server/mm_src.inc
include $(BUILDDIR)/server/rw_src.inc
include $(BUILDDIR)/server/rep_src.inc

SRCS += $(CACHE_SRC)
SRCS += $(CONF_SRC)
SRCS += $(DEV_SRC)
SRCS += $(TX_SRC)
SRCS += $(UMPK_SRC)
SRCS += $(MM_SRC)
SRCS += $(RW_SRC)
SRCS += $(REP_SRC)
SRCS += nid_log.c list_sort.c fpc.c \
	util_nw.c util_bdev.c bit.c mpk.c mpk_subr.c lck.c rtree.c \
	sah.c ds.c cds.c cdsg.c smc.c scg.c sacc.c saccg.c \
	tp.c tpg.c tpc.c tpcg.c\
	sds.c sdsg.c sdsc.c sdscg.c \
	hash.c \
	io.c rio.c bio.c \
	sac.c sacg.c doa.c doac.c doacg.c snap.c snapg.c smcg.c \
	nw.c nw_service.c \
	ini.c inic.c inicg.c \
	mqtt.c

SYMLINKS += $(CONF_SYMLINKS)
SYMLINKS += $(DEV_SYMLINKS)
SYMLINKS += $(TX_SYMLINKS)
SYMLINKS += $(UMPK_SYMLINKS)
SYMLINKS += $(MM_SYMLINKS)
SYMLINKS += $(RW_SYMLINKS)
SYMLINKS += $(CACHE_SYMLINKS)
SYMLINKS += $(REP_SYMLINKS)
SYMLINKS += version.h nid_shared.h nid.h list.h nid_log.h nid_log.c fpc.c fpc_if.h \
	list_sort.h list_sort.c \
	util_nw.h util_nw.c util_bdev.h util_bdev.c \
	bit_if.h bit.c \
	ini_if.h ini.c \
	lck_if.h lck.c \
	rtree_if.h rtree.c \
	tp_if.h tp.c tpg_if.h tpg.c tpc_if.h tpc.c tpcg_if.h tpcg.c \
	ds_if.h ds.c \
	cds_if.h cds.c cdsg_if.h cdsg.c \
	sds_if.h sds.c sdsg_if.h sdsg.c \
	smc_if.h smc.c \
	scg_if.h scg.c \
	io_if.h io.c \
	rio_if.h rio.c \
	bio_if.h bio.c \
	sah_if.h sah.c \
	nw.h nw_if.h nw.c nw_service.c \
	sac_if.h sac.c \
	smcg_if.h smcg.c \
	sacg_if.h sacg.c \
	doa_if.h doa.c \
	doac_if.h doac.c \
	doacg_if.h doacg.c \
	sacc_if.h sacc.c \
	saccg_if.h saccg.c \
	sdsc.c sdsc_if.h \
	sdscg.c sdscg_if.h \
	mpk_if.h mpk.c mpk_subr.c \
	snap_if.h snap.c \
	snapg_if.h snapg.c \
	hash_if.h hash.c \
	server.h \
	inic_if.h inic.c inicg_if.h inicg.c \
	mqtt_if.h mqtt.c

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h
	
bio.c:
	ln -f -s $(BASEDIR)/bio.c bio.c

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h
	
bit.c:
	ln -f -s $(BASEDIR)/bit.c bit.c
	
cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h
	
cds.c:
	ln -f -s $(BASEDIR)/cds.c cds.c
	
doa_if.h:
	ln -f -s $(BASEDIR)/doa_if.h doa_if.h
	
doa.c:
	ln -f -s $(BASEDIR)/doa.c doa.c

doac_if.h:
	ln -f -s $(BASEDIR)/doac_if.h doac_if.h
	
doac.c:
	ln -f -s $(BASEDIR)/doac.c doac.c

doacg_if.h:
	ln -f -s $(BASEDIR)/doacg_if.h doacg_if.h
	
doacg.c:
	ln -f -s $(BASEDIR)/doacg.c doacg.c

sacc.c:
	ln -f -s $(BASEDIR)/sacc.c sacc.c

sacc_if.h:
	ln -f -s $(BASEDIR)/sacc_if.h sacc_if.h

saccg.c:
	ln -f -s $(BASEDIR)/saccg.c saccg.c

saccg_if.h:
	ln -f -s $(BASEDIR)/saccg_if.h saccg_if.h

sdsc.c:
	ln -f -s $(BASEDIR)/sdsc.c sdsc.c

sdsc_if.h:
	ln -f -s $(BASEDIR)/sdsc_if.h sdsc_if.h

sdscg.c:
	ln -f -s $(BASEDIR)/sdscg.c sdscg.c

sdscg_if.h:
	ln -f -s $(BASEDIR)/sdscg_if.h sdscg_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h
	
ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c	

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h
	
fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

hash_if.h:
	ln -f -s $(BASEDIR)/hash_if.h hash_if.h

hash.c:
	ln -f -s $(BASEDIR)/hash.c hash.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h
	
ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h
	
io.c:
	ln -f -s $(BASEDIR)/io.c io.c	

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h
	
lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c	
	
mpk.c:
	ln -f -s $(BASEDIR)/../shared/mpk.c mpk.c

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mpk_subr.c:
	ln -f -s $(BASEDIR)/../shared/mpk_subr.c mpk_subr.c 

nw_if.h:
	ln -f -s $(BASEDIR)/nw_if.h nw_if.h

nw.h:
	ln -f -s $(BASEDIR)/nw.h nw.h

nw.c:
	ln -f -s $(BASEDIR)/nw.c nw.c

nw_service.c:
	ln -f -s $(BASEDIR)/nw_service.c nw_service.c	

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h
	
rio.c:
	ln -f -s $(BASEDIR)/rio.c rio.c		

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h
	
rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c
	
sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h
	
sac.c:
	ln -f -s $(BASEDIR)/sac.c sac.c
	
sacg_if.h:
	ln -f -s $(BASEDIR)/sacg_if.h sacg_if.h
	
sacg.c:
	ln -f -s $(BASEDIR)/sacg.c sacg.c

sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h
	
sah.c:
	ln -f -s $(BASEDIR)/sah.c sah.c	
	
scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h
	
scg.c:
	ln -f -s $(BASEDIR)/scg.c scg.c		

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h
	
sds.c:
	ln -f -s $(BASEDIR)/sds.c sds.c

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h
	
sdsg.c:
	ln -f -s $(BASEDIR)/sdsg.c sdsg.c

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

cdsg.c:
	ln -f -s $(BASEDIR)/cdsg.c cdsg.c

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h
	
smc.c:
	ln -f -s $(BASEDIR)/smc.c smc.c	

smcg_if.h:
	ln -f -s $(BASEDIR)/smcg_if.h smcg_if.h
	
smcg.c:
	ln -f -s $(BASEDIR)/smcg.c smcg.c

snap_if.h:
	ln -f -s $(BASEDIR)/snap_if.h snap_if.h
	
snap.c:
	ln -f -s $(BASEDIR)/snap.c snap.c

snapg_if.h:
	ln -f -s $(BASEDIR)/snapg_if.h snapg_if.h
	
snapg.c:
	ln -f -s $(BASEDIR)/snapg.c snapg.c
	
tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h
	
tpg.c:
	ln -f -s $(BASEDIR)/tpg.c tpg.c

tpc_if.h:
	ln -f -s $(BASEDIR)/tpc_if.h tpc_if.h
	
tpc.c:
	ln -f -s $(BASEDIR)/tpc.c tpc.c

tpcg_if.h:
	ln -f -s $(BASEDIR)/tpcg_if.h tpcg_if.h
	
tpcg.c:
	ln -f -s $(BASEDIR)/tpcg.c tpcg.c


util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h
	
util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

util_bdev.h:
	ln -f -s $(BASEDIR)/util_bdev.h util_bdev.h

util_bdev.c:
	ln -f -s $(BASEDIR)/util_bdev.c util_bdev.c

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

inic_if.h:
	ln -f -s $(BASEDIR)/inic_if.h inic_if.h

inic.c:
	ln -f -s $(BASEDIR)/inic.c inic.c

inicg_if.h:
	ln -f -s $(BASEDIR)/inicg_if.h inicg_if.h

inicg.c:
	ln -f -s $(BASEDIR)/inicg.c inicg.c

mqtt_if.h:
	ln -f -s $(BASEDIR)/mqtt_if.h mqtt_if.h

mqtt.c:
	ln -f -s $(BASEDIR)/mqtt.c mqtt.c
