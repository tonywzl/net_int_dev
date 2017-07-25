.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = nid_log.c dxpcg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h nid.h nid_log.h nid_log.c nid_shared.h \
	tp_if.h pp2_if.h \
	dxpg_if.h dxtg_if.h \
	lstn_if.h node_if.h list.h \
	txn_if.h \
	dxp_if.h \
	dxpcg_if.h dxpcg.c dxpc_if.h \
	scg_if.h sac_if.h tpg_if.h \
	io_if.h bio_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: dxpcg_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

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

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxpg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpg_if.h dxpg_if.h

dxpc_if.h:
	ln -f -s $(DX_BASEDIR)/dxpc_if.h dxpc_if.h

dxpcg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpcg_if.h dxpcg_if.h

dxpcg.c:
	ln -f -s $(DX_BASEDIR)/dxpcg.c dxpcg.c

dxtg_if.h:
	ln -f -s $(DX_BASEDIR)/dxtg_if.h dxtg_if.h
	
lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h	pp2_if.h

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h

dxpcg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxpcg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
