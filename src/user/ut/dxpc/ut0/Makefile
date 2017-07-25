.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = nid_log.c dxpc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h list.h tp_if.h nid_log.c nid_log.h nid_shared.h nid.h \
	umpk_if.h umpk_dxp_if.h umpk_dxa_if.h io_if.h bio_if.h \
	dxp_if.h dxpcg_if.h sac_if.h \
	dxpc_if.h dxpc.c dxpg_if.h tx_shared.h tpg_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxpc_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxpcg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpcg_if.h dxpcg_if.h

dxpc_if.h:
	ln -f -s $(DX_BASEDIR)/dxpc_if.h dxpc_if.h

dxpc.c:
	ln -f -s $(DX_BASEDIR)/dxpc.c dxpc.c

dxpg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpg_if.h dxpg_if.h

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

dxpc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxpc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
