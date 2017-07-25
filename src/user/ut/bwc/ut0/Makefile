.KEEP_STATE:

BASEDIR = ../../..
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wcast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7

SRCS = ut_main.c allocator.c \
	bwc.c bwc-ff.c bwc-dwr.c bwc-rec.c bwc-rec2.c bwc-snap.c bwc_rbn.c \
	pp.c lck.c nid_log.c

SYMLINKS = allocator.c allocator_if.h \
	bfe_if.h \
	bfp_if.h \
	bre_if.h \
	brn_if.h \
	bse_if.h \
	btn_if.h \
	bwc.c bwc_if.h bwc.h \
	bwc-ff.c bwc-dwr.c bwc-rec.c bwc-rec2.c bwc-snap.c \
	bwc_rbn.c bwc_rbn_if.h \
	wc_if.h \
	cdn_if.h \
	crc_if.h \
	d2an_if.h \
	d2bn_if.h \
	ddn_if.h \
	drw_if.h \
	ds_if.h \
	fpn_if.h \
	nid.h \
	nid_log.c nid_log.h \
	nid_shared.h \
	io_if.h \
	lck.c lck_if.h \
	list.h \
	lstn_if.h \
	node_if.h \
	pp_if.h pp.c \
	rc_if.h \
	rc_wc_cbn.h \
	rc_wcn.h \
	rtn_if.h \
	rw_if.h \
	sac_if.h \
	sds_if.h \
	srn_if.h \
	smc_if.h \
	tp_if.h \
	tpg_if.h \
	wcd_if.h \
	umpk_bwc_if.h umpk_crc_if.h \

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

bfp_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp_if.h bfp_if.h

bre_if.h:
	ln -f -s $(CACHE_BASEDIR)/bre_if.h bre_if.h

brn_if.h:
	ln -f -s $(MM_BASEDIR)/brn_if.h brn_if.h

bse_if.h:
	ln -f -s $(CACHE_BASEDIR)/bse_if.h bse_if.h

btn_if.h:
	ln -f -s $(MM_BASEDIR)/btn_if.h btn_if.h

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

wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h

cdn_if.h:
	ln -f -s $(MM_BASEDIR)/cdn_if.h cdn_if.h

crc_if.h:
	ln -f -s $(CACHE_BASEDIR)/crc_if.h crc_if.h

d2an_if.h:
	ln -f -s $(MM_BASEDIR)/d2an_if.h d2an_if.h

d2bn_if.h:
	ln -f -s $(MM_BASEDIR)/d2bn_if.h d2bn_if.h

ddn_if.h:
	ln -f -s $(MM_BASEDIR)/ddn_if.h ddn_if.h

drw_if.h:
	ln -f -s $(RW_BASEDIR)/drw_if.h drw_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

rc_if.h:
	ln -f -s $(CACHE_BASEDIR)/rc_if.h rc_if.h

rc_wc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_wc_cbn.h rc_wc_cbn.h

rc_wcn.h:
	ln -f -s $(MM_BASEDIR)/rc_wcn.h rc_wcn.h

rtn_if.h:
	ln -f -s $(MM_BASEDIR)/rtn_if.h rtn_if.h

rw_if.h:
	ln -f -s $(RW_BASEDIR)/rw_if.h rw_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

srn_if.h:
	ln -f -s $(MM_BASEDIR)/srn_if.h srn_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h

umpk_crc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc_if.h umpk_crc_if.h

wcd_if.h:
	ln -f -s $(CACHE_BASEDIR)/wcd_if.h wcd_if.h

ut_main: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut_main *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
