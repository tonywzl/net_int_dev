.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c nid_log.c  cxac.c cxacg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c nid.h nid_log.h nid_log.c nid_shared.h version.h \
	tp_if.h pp2_if.h \
	cxag_if.h cxtg_if.h \
	tx_shared.h \
	umpk_if.h umpk.h \
	umpk_cxa_if.h umpk_cxa_if.h \
	lstn_if.h node_if.h list.h \
	txn_if.h \
	ds_if.h sdsg_if.h cdsg_if.h \
	cxa_if.h \
	cxacg_if.h cxacg.c cxac_if.h \
	scg_if.h sac_if.h tpg_if.h \
	io_if.h bio_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: cxacg_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

cxa_if.h:
	ln -f -s $(DX_BASEDIR)/cxa_if.h cxa_if.h

cxag_if.h:
	ln -f -s $(DX_BASEDIR)/cxag_if.h cxag_if.h

cxac_if.h:
	ln -f -s $(DX_BASEDIR)/cxac_if.h cxac_if.h

cxac.c:
	ln -f -s $(DX_BASEDIR)/cxac.c cxac.c

cxacg_if.h:
	ln -f -s $(DX_BASEDIR)/cxacg_if.h cxacg_if.h

cxacg.c:
	ln -f -s $(DX_BASEDIR)/cxacg.c cxacg.c

cxtg_if.h:
	ln -f -s $(DX_BASEDIR)/cxtg_if.h cxtg_if.h
	
tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

umpk.h:
	ln -f -s $(UMPK_BASEDIR)/umpk.h umpk.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h	pp2_if.h

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h

cxacg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cxacg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
