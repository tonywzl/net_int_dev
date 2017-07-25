.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include

ifdef RELEASE
LIBPATH = -L../../../../metaserver/release
else
LIBPATH = -L../../../../metaserver/debug
endif

LIBPATH += -lglib-2.0 -L../../../../metaserver/opensrc

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7 -DMETASERVER

SYSLIBS = -lpthread -lcrypto -lms -lcom -lrados -lcassandra -lpthread -lstdc++ -lm -lboost_program_options -lboost_thread -lboost_system -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -ldl -luuid -llttng-ust -luv -lssh2 -lcrypto -lglib-2.0

SRCS = ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = ms_intf.h msret.h ioflag.h
#SYMLINKS = atomic.h nid.h nid_log.h nid_log.c nid_shared.h list.h version.h \
	ini_if.h ini.c \
	tp_if.h mq_if.h pp2_if.h \
	allocator_if.h allocator.c \
	dxa_if.h dxa.c \
	dxp_if.h dxpg_if.h \
	dxag_if.h dxag.c \
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
	dxt_if.h dxt.c \
	util_nw.h util_nw.c \
	txn_if.h txn.c \
	server.h \
	io_if.h \
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
	sdsg_if.h sdsg.c cdsg_if.h cdsg.c \
	sds_if.h sds.c cds_if.h cds.c \
	ds_if.h ds.c \
	ppg_if.h ppg.c \
	reps_conf.c rept_conf.c	\
	reps.c reps_if.h rept_if.h \
	ms_intf.h msret.h ioflag.h


TOPDIR = ../../../../..
BASEDIR = ../../..
CONF_BASEDIR = ../../../conf
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk
REP_BASEDIR = ../../../rep

all: reps_mds_write_data_unit

ms_intf.h:
	ln -f -s $(BASEDIR)/../metaserver/include/ms_intf.h ms_intf.h

msret.h:
	ln -f -s $(BASEDIR)/../metaserver/include/msret.h msret.h

ioflag.h:
	ln -f -s $(BASEDIR)/../metaserver/include/ioflag.h ioflag.h

reps_mds_write_data_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f reps_mds_write_data_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
