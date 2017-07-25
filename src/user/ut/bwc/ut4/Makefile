.KEEP_STATE:

BASEDIR = ../../..
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto -lm

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wcast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7

SRCS = ut_main.c allocator.c bfe.c \
	bfe-async.c bfe-sync.c bfp.c bfp1.c bfp2.c bfp3.c bfp4.c bfp5.c \
	bio.c bit.c blksn.c bre.c brn.c bse.c btn.c \
	bwc.c bwc-ff.c bwc-dwr.c bwc-rec.c bwc-rec2.c bwc-snap.c bwc_rbn.c \
	cdn.c cds.c crc.c cse.c cse_crc_cbn.c  cse_mds_cbn.c \
	d1an.c d2an.c d2bn.c d2cn.c ddn.c drw.c ds.c dsrec.c dsmgr.c dsbmp.c \
	fpc.c fpn.c sac.c pp.c pp2.c hash.c \
	nid_log.c io.c list_sort.c lck.c lstn.c \
	mrw.c mrw_wn.c mrw_wsn.c mrw_rn.c \
	node.c nse_crc_cbn.c nse_mds_cbn.c nse_mdsn.c nse.c \
	rio.c rtree.c rtn.c rc.c rc_wc_cbn.c rc_wcn.c rw.c \
	sds.c srn.c tp.c wc.c twc.c  \

SYMLINKS = allocator.c allocator_if.h \
	bfe.c bfe_if.h bfe-async.c bfe-sync.c \
	bfp.c bfp_if.h \
	bfp1.c bfp1_if.h \
	bfp2.c bfp2_if.h \
	bfp3.c bfp3_if.h \
	bfp4.c bfp4_if.h \
	bfp5.c bfp5_if.h \
	bio.c bio_if.h \
	bit.c bit_if.h \
	blksn.c blksn_if.h \
	bre.c bre_if.h \
	brn.c brn_if.h \
	bse.c bse_if.h \
	bsn_if.h \
	btn.c btn_if.h \
	bwc.c bwc_if.h bwc.h \
	bwc-ff.c bwc-dwr.c bwc-rec.c bwc-rec2.c bwc-snap.c \
	bwc_rbn.c bwc_rbn_if.h \
	cdn.c cdn_if.h \
	cds.c cds_if.h \
	cdsg_if.h \
	crc.c crc_if.h \
	cse.c cse_if.h  \
	cse_crc_cbn.c cse_crc_cbn.h \
	cse_mds_cbn.c cse_mds_cbn.h \
	d1an_if.h d1an.c \
	d2an.c d2an_if.h \
	d2bn.c d2bn_if.h \
	d2cn.c d2cn_if.h \
	ddn.c ddn_if.h \
	drw_if.h \
	ds.c ds_if.h \
	dsrec.c dsrec_if.h \
	dsbmp.c dsbmp_if.h \
	dsmgr.c dsmgr_if.h \
	fpc.c fpc_if.h \
	fpn.c fpn_if.h \
	hash.c hash_if.h \
	nid.h \
	nid_log.c nid_log.h \
	nid_shared.h \
	ini_if.h \
	io.c io_if.h \
	lck.c lck_if.h \
	list.h \
	list_sort.c list_sort.h \
	lstn.c lstn_if.h \
	mrw.c mrw_if.h \
	mrw_rn.c mrw_rn_if.h \
	mrw_wn.c mrw_wn_if.h \
	mrw_wsn.c mrw_wsn_if.h \
	node.c node_if.h \
	nse.c nse_if.h \
	nse_crc_cbn.c nse_crc_cbn.h \
	nse_mdsn.c nse_mdsn.h \
	nse_mds_cbn.c nse_mds_cbn.h \
	pp_if.h pp.c \
	pp2.c pp2_if.h \
	rc.c rc_if.h \
	rc_wc_cbn.c rc_wc_cbn.h \
	rc_wcn.c rc_wcn.h \
	rio.c rio_if.h \
	rtree.c rtree_if.h \
	rtn.c rtn_if.h \
	rw.c rw_if.h \
	sdsg_if.h \
	sac_if.h \
	sds.c sds_if.h \
	srn.c srn_if.h \
	server.h \
	smc_if.h \
	twc_if.h twc.c \
	tp.c tp_if.h \
	tpg_if.h \
	wc.c wc_if.h \
	umpk_twc_if.h umpk_bwc_if.h umpk_crc_if.h \

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
MM_BASEDIR = ../../../mm
CACHE_BASEDIR = ../../../cache
RW_BASEDIR = ../../../rw
UMPK_BASEDIR = ../../../umpk

OBJS = $(SRCS:.c=.o)

all: ut_main

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(MM_BASEDIR)/allocator.c allocator.c

bfe_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfe_if.h bfe_if.h

bfe.c:
	ln -f -s $(CACHE_BASEDIR)/bfe.c bfe.c

bfe-async.c:
	ln -f -s $(CACHE_BASEDIR)/bfe-async.c bfe-async.c

bfe-sync.c:
	ln -f -s $(CACHE_BASEDIR)/bfe-sync.c bfe-sync.c

bfp_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp_if.h bfp_if.h

bfp.c:
	ln -f -s $(CACHE_BASEDIR)/bfp.c bfp.c

bfp1_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp1_if.h bfp1_if.h

bfp1.c:
	ln -f -s $(CACHE_BASEDIR)/bfp1.c bfp1.c

bfp2_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp2_if.h bfp2_if.h

bfp2.c:
	ln -f -s $(CACHE_BASEDIR)/bfp2.c bfp2.c

bfp3_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp3_if.h bfp3_if.h

bfp3.c:
	ln -f -s $(CACHE_BASEDIR)/bfp3.c bfp3.c

bfp4_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp4_if.h bfp4_if.h

bfp4.c:
	ln -f -s $(CACHE_BASEDIR)/bfp4.c bfp4.c

bfp5_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp5_if.h bfp5_if.h

bfp5.c:
	ln -f -s $(CACHE_BASEDIR)/bfp5.c bfp5.c

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

bio.c:
	ln -s -s $(BASEDIR)/bio.c bio.c

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h

bit.c:
	ln -f -s $(BASEDIR)/bit.c bit.c

blksn_if.h:
	ln -f -s $(MM_BASEDIR)/blksn_if.h blksn_if.h

blksn.c:
	ln -f -s $(MM_BASEDIR)/blksn.c blksn.c

bre_if.h:
	ln -f -s $(CACHE_BASEDIR)/bre_if.h bre_if.h

bre.c:
	ln -f -s $(CACHE_BASEDIR)/bre.c bre.c

brn_if.h:
	ln -f -s $(MM_BASEDIR)/brn_if.h brn_if.h

brn.c:
	ln -f -s $(MM_BASEDIR)/brn.c brn.c

bse_if.h:
	ln -f -s $(CACHE_BASEDIR)/bse_if.h bse_if.h

bse.c:
	ln -f -s $(CACHE_BASEDIR)/bse.c bse.c

bsn_if.h:
	ln -f -s $(MM_BASEDIR)/bsn_if.h bsn_if.h

btn_if.h:
	ln -f -s $(MM_BASEDIR)/btn_if.h btn_if.h

btn.c:
	ln -f -s $(MM_BASEDIR)/btn.c btn.c

bwc.c:
	ln -f -s $(CACHE_BASEDIR)/bwc.c bwc.c

bwc.h:
	ln -f -s $(CACHE_BASEDIR)/bwc.h bwc.h

bwc_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwc_if.h bwc_if.h

bwc_rbn.c:
	ln -f -s $(MM_BASEDIR)/bwc_rbn.c bwc_rbn.c

bwc_rbn_if.h:
	ln -f -s $(MM_BASEDIR)/bwc_rbn_if.h bwc_rbn_if.h

bwc-ff.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-ff.c bwc-ff.c

bwc-dwr.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-dwr.c bwc-dwr.c

bwc-rec2.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-rec2.c bwc-rec2.c

bwc-rec.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-rec.c bwc-rec.c

bwc-snap.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-snap.c bwc-snap.c

cdn_if.h:
	ln -f -s $(MM_BASEDIR)/cdn_if.h cdn_if.h

cdn.c:
	ln -f -s $(MM_BASEDIR)/cdn.c cdn.c

cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h

cds.c:
	ln -f -s $(BASEDIR)/cds.c cds.c

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

crc_if.h:
	ln -f -s $(CACHE_BASEDIR)/crc_if.h crc_if.h

crc.c:
	ln -f -s $(CACHE_BASEDIR)/crc.c crc.c

cse_if.h:
	ln -f -s $(CACHE_BASEDIR)/cse_if.h cse_if.h

cse.c:
	ln -f -s $(CACHE_BASEDIR)/cse.c cse.c

cse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.h cse_crc_cbn.h

cse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.c cse_crc_cbn.c

cse_mds_cbn.h:
	ln -f -s $(MM_BASEDIR)/cse_mds_cbn.h cse_mds_cbn.h

cse_mds_cbn.c:
	ln -f -s $(MM_BASEDIR)/cse_mds_cbn.c cse_mds_cbn.c

d1an_if.h:
	ln -f -s $(MM_BASEDIR)/d1an_if.h d1an_if.h

d1an.c:
	ln -f -s $(MM_BASEDIR)/d1an.c d1an.c

d2an_if.h:
	ln -f -s $(MM_BASEDIR)/d2an_if.h d2an_if.h

d2an.c:
	ln -f -s $(MM_BASEDIR)/d2an.c d2an.c

d2bn_if.h:
	ln -f -s $(MM_BASEDIR)/d2bn_if.h d2bn_if.h

d2bn.c:
	ln -f -s $(MM_BASEDIR)/d2bn.c d2bn.c

d2cn_if.h:
	ln -f -s $(MM_BASEDIR)/d2cn_if.h d2cn_if.h

d2cn.c:
	ln -f -s $(MM_BASEDIR)/d2cn.c d2cn.c

ddn_if.h:
	ln -f -s $(MM_BASEDIR)/ddn_if.h ddn_if.h

ddn.c:
	ln -f -s $(MM_BASEDIR)/ddn.c ddn.c

drw_if.h:
	ln -f -s $(RW_BASEDIR)/drw_if.h drw_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c

dsbmp_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsbmp_if.h dsbmp_if.h

dsbmp.c:
	ln -f -s $(CACHE_BASEDIR)/dsbmp.c dsbmp.c
dsmgr.c:
	ln -f -s $(CACHE_BASEDIR)/dsmgr.c dsmgr.c

dsmgr_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsmgr_if.h dsmgr_if.h

dsrec.c:
	ln -f -s $(CACHE_BASEDIR)/dsrec.c dsrec.c

dsrec_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsrec_if.h dsrec_if.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h

fpn.c:
	ln -f -s $(MM_BASEDIR)/fpn.c fpn.c

hash_if.h:
	ln -f -s $(BASEDIR)/hash_if.h hash_if.h

hash.c:
	ln -f -s $(BASEDIR)/hash.c hash.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

io.c:
	ln -s -s $(BASEDIR)/io.c io.c

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


lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

lstn.c:
	ln -f -s $(MM_BASEDIR)/lstn.c lstn.c

mrw_if.h:
	ln -f -s $(RW_BASEDIR)/mrw_if.h mrw_if.h

mrw.c:
	ln -f -s $(RW_BASEDIR)/mrw.c mrw.c

mrw_rn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_rn_if.h mrw_rn_if.h

mrw_rn.c:
	ln -s -s $(MM_BASEDIR)/mrw_rn.c mrw_rn.c

mrw_wn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_wn_if.h mrw_wn_if.h

mrw_wn.c:
	ln -f -s $(MM_BASEDIR)/mrw_wn.c mrw_wn.c

mrw_wsn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_wsn_if.h mrw_wsn_if.h

mrw_wsn.c:
	ln -f -s $(MM_BASEDIR)/mrw_wsn.c mrw_wsn.c

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

node.c:
	ln -f -s $(MM_BASEDIR)/node.c node.c

nse_if.h:
	ln -f -s $(CACHE_BASEDIR)/nse_if.h nse_if.h

nse.c:
	ln -f -s $(CACHE_BASEDIR)/nse.c nse.c

nse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.h nse_crc_cbn.h

nse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.c nse_crc_cbn.c

nse_mdsn.c:
	ln -f -s $(MM_BASEDIR)/nse_mdsn.c nse_mdsn.c

nse_mdsn.h:
	ln -f -s $(MM_BASEDIR)/nse_mdsn.h nse_mdsn.h

nse_mds_cbn.h:
	ln -f -s $(MM_BASEDIR)/nse_mds_cbn.h nse_mds_cbn.h

nse_mds_cbn.c:
	ln -f -s $(MM_BASEDIR)/nse_mds_cbn.c nse_mds_cbn.c

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(MM_BASEDIR)/pp2.c pp2.c

rc_if.h:
	ln -f -s $(CACHE_BASEDIR)/rc_if.h rc_if.h

rc.c:
	ln -f -s $(CACHE_BASEDIR)/rc.c rc.c

rc_wc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_wc_cbn.h rc_wc_cbn.h

rc_wc_cbn.c:
	ln -f -s $(MM_BASEDIR)/rc_wc_cbn.c rc_wc_cbn.c

rc_wcn.h:
	ln -f -s $(MM_BASEDIR)/rc_wcn.h rc_wcn.h

rc_wcn.c:
	ln -f -s $(MM_BASEDIR)/rc_wcn.c rc_wcn.c

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h

rio.c:
	ln -s -s $(BASEDIR)/rio.c rio.c

rtn_if.h:
	ln -f -s $(MM_BASEDIR)/rtn_if.h rtn_if.h

rtn.c:
	ln -f -s $(MM_BASEDIR)/rtn.c rtn.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

rw_if.h:
	ln -f -s $(RW_BASEDIR)/rw_if.h rw_if.h

rw.c:
	ln -f -s $(RW_BASEDIR)/rw.c rw.c

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h

sds.c:
	ln -f -s $(BASEDIR)/sds.c sds.c

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

srn_if.h:
	ln -f -s $(MM_BASEDIR)/srn_if.h srn_if.h

srn.c:
	ln -f -s $(MM_BASEDIR)/srn.c srn.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

twc_if.h:
	ln -f -s $(CACHE_BASEDIR)/twc_if.h twc_if.h

twc.c:
	ln -f -s $(CACHE_BASEDIR)/twc.c twc.c

umpk_twc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc_if.h umpk_twc_if.h

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h

umpk_crc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc_if.h umpk_crc_if.h

wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h

wc.c:
	ln -f -s $(CACHE_BASEDIR)/wc.c wc.c


ut_main: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut_main *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
