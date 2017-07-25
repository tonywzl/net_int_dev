.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include

ifdef DEBUG
LIBPATH = -L../../../../metaserver/debug
else
LIBPATH = -L../../../../metaserver/release
endif

LIBPATH += -lglib-2.0 -L../../../../metaserver/opensrc

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7 -DMETASERVER

SYSLIBS = -lpthread -lcrypto -lms -lcom -lrados -lcassandra -lpthread -lstdc++ -lm -lboost_program_options -lboost_thread -lboost_system -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -ldl -luuid -llttng-ust -luv -lssh2 -lcrypto -lglib-2.0

SRCS =  nid_log.c dxa.c dxag.c pp.c pp2.c tp.c lck.c lstn.c \
	umpk_bwc.c umpk.c umpk_cxa.c umpk_cxp.c \
	umpk_dck.c umpk_doa.c umpk_drc.c umpk_drw.c \
	umpk_dxa.c umpk_dxp.c umpk_ini.c umpk_mrw.c \
	umpk_pp.c umpk_crc.c umpk_reps.c \
	umpk_rept.c umpk_sac.c umpk_sds.c umpk_shm.c \
	umpk_tp.c umpk_twc.c umpk_wc.c tx_shared.c \
	node.c allocator.c dxtg.c dxt.c util_nw.c ini.c \
	server_conf.c server_global_conf.c cds_conf.c \
	crc_conf.c cxa_conf.c cxp_conf.c cxt_conf.c \
	drc_conf.c dxa_conf.c dxp_conf.c dxt_conf.c \
	bwc_conf.c pp_conf.c sac_conf.c \
	scg_conf.c sds_conf.c tp_conf.c twc_conf.c \
	dck_conf.c shm_conf.c shr_conf.c rgdev_conf.c \
	drw_conf.c mrw_conf.c sarw_conf.c rsa_conf.c \
	rsm_conf.c rs_conf.c reps_conf.c rept_conf.c \
	ut_main.c reptc.c reptcg.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = atomic.h nid.h nid_log.h nid_log.c nid_shared.h list.h \
	ini_if.h ini.c \
	tp_if.h mq_if.h pp2_if.h \
	allocator_if.h allocator.c \
	dxa_if.h dxa.c \
	dxp_if.h dxpg_if.h \
	dxag_if.h dxag.c \
	umpk_if.h umpk_dxa_if.h \
	dxtg_if.h dxtg.c \
	pp_if.h pp.c \
	pp2_if.h pp2.c \
	tp_if.h tp.c \
	lck_if.h lck.c \
	lstn_if.h lstn.c \
	node_if.h  node.c \
	umpk_if.h umpk.h umpk.c \
	umpk_bwc_if.h umpk_bwc.c \
	umpk_cxa_if.h umpk_cxa.c \
	umpk_cxp_if.h umpk_cxp.c \
	umpk_dck_if.h umpk_dck.c \
	umpk_doa_if.h umpk_doa.c \
	umpk_drc_if.h umpk_drc.c \
	umpk_drw_if.h umpk_drw.c \
	umpk_dxa_if.h umpk_dxa.c \
	umpk_dxp_if.h umpk_dxp.c \
	umpk_ini_if.h umpk_ini.c \
	umpk_mrw_if.h umpk_mrw.c \
	umpk_pp_if.h umpk_pp.c \
	umpk_crc_if.h umpk_crc.c \
	umpk_reps_if.h umpk_reps.c \
	umpk_rept_if.h umpk_rept.c \
	umpk_sac_if.h umpk_sac.c \
	umpk_sds_if.h umpk_sds.c \
	umpk_shm_if.h umpk_shm.c \
	umpk_tp_if.h umpk_tp.c \
	umpk_twc_if.h umpk_twc.c \
	umpk_wc_if.h umpk_wc.c \
	tx_shared.h tx_shared.c \
	ds_if.h sdsg_if.h cdsg_if.h \
	dxt_if.h dxt.c \
	util_nw.h util_nw.c \
	txn_if.h \
	server.h \
	server_conf.c server_global_conf.c \
	cds_conf.c crc_conf.c cxa_conf.c \
	cxp_conf.c cxt_conf.c drc_conf.c \
        dxa_conf.c dxp_conf.c dxt_conf.c \
        bwc_conf.c pp_conf.c \
        sac_conf.c scg_conf.c sds_conf.c \
        tp_conf.c twc_conf.c dck_conf.c \
        shm_conf.c shr_conf.c rgdev_conf.c \
	drw_conf.c mrw_conf.c sarw_conf.c \
	rsa_conf.c rsm_conf.c rs_conf.c \
	reps_conf.c rept_conf.c	\
	reps_if.h rept_if.h reptg_if.h \
	reptc.c reptc_if.h reptcg_if.h reptcg.c \
	version.h \
	ms_intf.h msret.h ioflag.h

BASEDIR = ../../..
CONF_BASEDIR = ../../../conf
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk
REP_BASEDIR = ../../../rep

all: reptcg_unit

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(MM_BASEDIR)/allocator.c allocator.c

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(MM_BASEDIR)/pp2.c pp2.c

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

node.c:
	ln -f -s $(MM_BASEDIR)/node.c node.c

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

lstn.c:
	ln -f -s $(MM_BASEDIR)/lstn.c lstn.c

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

dxa_if.h:
	ln -f -s $(DX_BASEDIR)/dxa_if.h dxa_if.h

dxa.c:
	ln -f -s $(DX_BASEDIR)/dxa.c dxa.c
dxag_if.h:
	ln -f -s $(DX_BASEDIR)/dxag_if.h dxag_if.h

dxag.c:
	ln -f -s $(DX_BASEDIR)/dxag.c dxag.c

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxpg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpg_if.h dxpg_if.h

dxtg_if.h:
	ln -f -s $(DX_BASEDIR)/dxtg_if.h dxtg_if.h

dxtg.c:
	ln -f -s $(DX_BASEDIR)/dxtg.c dxtg.c

dxt_if.h:
	ln -f -s $(DX_BASEDIR)/dxt_if.h dxt_if.h

dxt.c:
	ln -f -s $(DX_BASEDIR)/dxt.c dxt.c

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

tx_shared.c:
	ln -f -s $(DX_BASEDIR)/tx_shared.c tx_shared.c

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk.h:
	ln -f -s $(UMPK_BASEDIR)/umpk.h umpk.h

umpk.c:
	ln -f -s $(UMPK_BASEDIR)/umpk.c umpk.c

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h

umpk_bwc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc.c umpk_bwc.c

umpk_crc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc_if.h umpk_crc_if.h

umpk_crc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc.c umpk_crc.c

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h

umpk_cxa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa.c umpk_cxa.c

umpk_cxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp_if.h umpk_cxp_if.h

umpk_cxp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp.c umpk_cxp.c

umpk_dck_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dck_if.h umpk_dck_if.h

umpk_dck.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dck.c umpk_dck.c

umpk_doa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_doa_if.h umpk_doa_if.h

umpk_doa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_doa.c umpk_doa.c

umpk_drc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drc_if.h umpk_drc_if.h

umpk_drc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_drc.c umpk_drc.c

umpk_drw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drw_if.h umpk_drw_if.h

umpk_drw.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_drw.c umpk_drw.c

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

umpk_dxa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa.c umpk_dxa.c

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

umpk_dxp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp.c umpk_dxp.c

umpk_ini_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_ini_if.h umpk_ini_if.h

umpk_ini.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_ini.c umpk_ini.c

umpk_mrw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_mrw_if.h umpk_mrw_if.h

umpk_mrw.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_mrw.c umpk_mrw.c

umpk_pp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_pp_if.h umpk_pp_if.h

umpk_pp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_pp.c umpk_pp.c


umpk_reps_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_reps_if.h umpk_reps_if.h

umpk_reps.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_reps.c umpk_reps.c

umpk_rept_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_rept_if.h umpk_rept_if.h

umpk_rept.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_rept.c umpk_rept.c

umpk_sac_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sac_if.h umpk_sac_if.h

umpk_sac.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_sac.c umpk_sac.c

umpk_sds_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sds_if.h umpk_sds_if.h

umpk_sds.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_sds.c umpk_sds.c

umpk_shm_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_shm_if.h umpk_shm_if.h

umpk_shm.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_shm.c umpk_shm.c

umpk_tp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_tp_if.h umpk_tp_if.h

umpk_tp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_tp.c umpk_tp.c

umpk_twc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc_if.h umpk_twc_if.h

umpk_twc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc.c umpk_twc.c

umpk_wc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc_if.h umpk_wc_if.h

umpk_wc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc.c umpk_wc.c

server_conf.c:
	ln -f -s $(CONF_BASEDIR)/server_conf.c server_conf.c

server_global_conf.c:
	ln -f -s $(CONF_BASEDIR)/server_global_conf.c server_global_conf.c

cds_conf.c:
	ln -f -s $(CONF_BASEDIR)/cds_conf.c cds_conf.c

crc_conf.c:
	ln -f -s $(CONF_BASEDIR)/crc_conf.c crc_conf.c

cxa_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxa_conf.c cxa_conf.c

cxp_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxp_conf.c cxp_conf.c

cxt_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxt_conf.c cxt_conf.c

drc_conf.c:
	ln -f -s $(CONF_BASEDIR)/drc_conf.c drc_conf.c

dxa_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxa_conf.c dxa_conf.c

dxp_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxp_conf.c dxp_conf.c

dxt_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxt_conf.c dxt_conf.c

bwc_conf.c:
	ln -f -s $(CONF_BASEDIR)/bwc_conf.c bwc_conf.c

pp_conf.c:
	ln -f -s $(CONF_BASEDIR)/pp_conf.c pp_conf.c

sac_conf.c:
	ln -f -s $(CONF_BASEDIR)/sac_conf.c sac_conf.c

scg_conf.c:
	ln -f -s $(CONF_BASEDIR)/scg_conf.c scg_conf.c

sds_conf.c:
	ln -f -s $(CONF_BASEDIR)/sds_conf.c sds_conf.c

tp_conf.c:
	ln -f -s $(CONF_BASEDIR)/tp_conf.c tp_conf.c

twc_conf.c:
	ln -f -s $(CONF_BASEDIR)/twc_conf.c twc_conf.c

dck_conf.c:
	ln -f -s $(CONF_BASEDIR)/dck_conf.c dck_conf.c

shm_conf.c:
	ln -f -s $(CONF_BASEDIR)/shm_conf.c shm_conf.c

shr_conf.c:
	ln -f -s $(CONF_BASEDIR)/shr_conf.c shr_conf.c

rgdev_conf.c:
	ln -f -s $(CONF_BASEDIR)/rgdev_conf.c rgdev_conf.c

drw_conf.c:
	ln -f -s $(CONF_BASEDIR)/drw_conf.c drw_conf.c

mrw_conf.c:
	ln -f -s $(CONF_BASEDIR)/mrw_conf.c mrw_conf.c

sarw_conf.c:
	ln -f -s $(CONF_BASEDIR)/sarw_conf.c sarw_conf.c

rsa_conf.c:
	ln -f -s $(CONF_BASEDIR)/rsa_conf.c rsa_conf.c

rsm_conf.c:
	ln -f -s $(CONF_BASEDIR)/rsm_conf.c rsm_conf.c

rs_conf.c:
	ln -f -s $(CONF_BASEDIR)/rs_conf.c rs_conf.c

reps_conf.c:
	ln -f -s $(CONF_BASEDIR)/reps_conf.c reps_conf.c

rept_conf.c:
	ln -f -s $(CONF_BASEDIR)/rept_conf.c rept_conf.c

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

ms_intf.h:
	ln -f -s $(BASEDIR)/../metaserver/include/ms_intf.h ms_intf.h

msret.h:
	ln -f -s $(BASEDIR)/../metaserver/include/msret.h msret.h

ioflag.h:
	ln -f -s $(BASEDIR)/../metaserver/include/ioflag.h ioflag.h

reps_if.h:
	ln -f -s $(REP_BASEDIR)/reps_if.h reps_if.h

reptg_if.h:
	ln -f -s $(REP_BASEDIR)/reptg_if.h reptg_if.h

rept_if.h:
	ln -f -s $(REP_BASEDIR)/rept_if.h rept_if.h

reptc_if.h:
	ln -f -s $(REP_BASEDIR)/reptc_if.h reptc_if.h

reptc.c:
	ln -f -s $(REP_BASEDIR)/reptc.c reptc.c

reptcg_if.h:
	ln -f -s $(REP_BASEDIR)/reptcg_if.h reptcg_if.h

reptcg.c:
	ln -f -s $(REP_BASEDIR)/reptcg.c reptcg.c


reptcg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f reptcg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
