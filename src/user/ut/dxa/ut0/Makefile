.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c nid_log.c dxa.c dxt.c tx_shared.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c nid.h nid_log.h nid_log.c nid_shared.h list.h \
	dxa_if.h dxa.c \
	dxt_if.h dxt.c \
	tp_if.h pp2_if.h \
	dxag_if.h dxtg_if.h \
	tx_shared.h tx_shared.c \
	umpk_if.h umpk.h \
	umpk_bwc_if.h \
	umpk_cxa_if.h \
	umpk_cxp_if.h \
	umpk_dck_if.h \
	umpk_doa_if.h \
	umpk_drc_if.h \
	umpk_drw_if.h \
	umpk_dxa_if.h \
	umpk_dxp_if.h \
	umpk_ini_if.h \
	umpk_mrw_if.h \
	umpk_bwc_if.h \
	umpk_pp_if.h \
	umpk_crc_if.h \
	umpk_reps_if.h \
	umpk_rept_if.h \
	umpk_sac_if.h \
	umpk_sds_if.h \
	umpk_shm_if.h \
	umpk_tp_if.h \
	umpk_twc_if.h \
	umpk_wc_if.h \
	lstn_if.h \
	node_if.h \
	txn_if.h \
	ds_if.h sdsg_if.h cdsg_if.h \
	dxp_if.h \
	version.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxa_unit
	
version.h:
	ln -f -s $(BASEDIR)/../../build/version.h
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

dxa_if.h:
	ln -f -s $(DX_BASEDIR)/dxa_if.h dxa_if.h

dxa.c:
	ln -f -s $(DX_BASEDIR)/dxa.c dxa.c

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxag_if.h:
	ln -f -s $(DX_BASEDIR)/dxag_if.h dxag_if.h

dxtg_if.h:
	ln -f -s $(DX_BASEDIR)/dxtg_if.h dxtg_if.h

	
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

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h

umpk_crc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc_if.h umpk_crc_if.h

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h

umpk_cxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp_if.h umpk_cxp_if.h

umpk_dck_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dck_if.h umpk_dck_if.h

umpk_doa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_doa_if.h umpk_doa_if.h

umpk_drc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drc_if.h umpk_drc_if.h

umpk_drw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drw_if.h umpk_drw_if.h

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

umpk_ini_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_ini_if.h umpk_ini_if.h

umpk_mrw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_mrw_if.h umpk_mrw_if.h

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h

umpk_pp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_pp_if.h umpk_pp_if.h

umpk_reps_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_reps_if.h umpk_reps_if.h

umpk_rept_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_rept_if.h umpk_rept_if.h

umpk_sac_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sac_if.h umpk_sac_if.h

umpk_sds_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sds_if.h umpk_sds_if.h

umpk_shm_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_shm_if.h umpk_shm_if.h

umpk_tp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_tp_if.h umpk_tp_if.h

umpk_twc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc_if.h umpk_twc_if.h

umpk_wc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc_if.h umpk_wc_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h	pp2_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h

dxa_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxa_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
