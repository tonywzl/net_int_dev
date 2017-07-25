.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c nid_log.c cxa.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c nid.h nid_log.h nid_log.c nid_shared.h list.h \
	cxa_if.h cxa.c cxt_if.h cxtg_if.h cxag_if.h \
	umpk_if.h umpk_cxa_if.h umpk_cxp_if.h \
	lstn_if.h node_if.h tx_shared.h version.h

TOPDIR = ../../../../..
BASEDIR = ../../..
CONF_BASEDIR = ../../../conf
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: cxa_unit

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h
	
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

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

cxa_if.h:
	ln -f -s $(BASEDIR)/tx/cxa_if.h cxa_if.h

cxa.c:
	ln -f -s $(BASEDIR)/tx/cxa.c cxa.c

cxt_if.h:
	ln -f -s $(BASEDIR)/tx/cxt_if.h cxt_if.h

cxtg_if.h:
	ln -f -s $(BASEDIR)/tx/cxtg_if.h cxtg_if.h

cxag_if.h:
	ln -f -s $(BASEDIR)/tx/cxag_if.h cxag_if.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h

umpk_cxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp_if.h umpk_cxp_if.h

cxa_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cxa_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
