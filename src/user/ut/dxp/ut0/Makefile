.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c nid_log.c dxp.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c nid.h nid_log.h nid_log.c nid_shared.h list.h \
	tx_shared.h dxp_if.h dxp.c dxt_if.h dxtg_if.h dxpg_if.h \
	umpk_if.h umpk.h umpk_dxp_if.h \
	lstn_if.h node_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxp_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxp.c:
	ln -f -s $(DX_BASEDIR)/dxp.c dxp.c

dxpg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpg_if.h dxpg_if.h

dxt_if.h:
	ln -f -s $(DX_BASEDIR)/dxt_if.h dxt_if.h

dxtg_if.h:
	ln -f -s $(DX_BASEDIR)/dxtg_if.h dxtg_if.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk.h:
	ln -f -s $(UMPK_BASEDIR)/umpk.h umpk.h

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

dxp_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxp_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
