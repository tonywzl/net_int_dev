.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dxt.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h util_nw.h nid_log.h nid_log.c \
	nid_shared.h list.h version.h \
	node_if.h lstn_if.h txn_if.h pp2_if.h \
	tx_shared.h \
	ds_if.h \
	dxt_if.h dxt.c \
	dxa_if.h dxp_if.h

TOPDIR = ../../../../..
BASEDIR = ../../..
TX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: dxt_unit

version.h:
	ln -f -s $(TOPDIR)/build/version.h version.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

dxt_if.h:
	ln -f -s $(TX_BASEDIR)/dxt_if.h dxt_if.h

dxt.c:
	ln -f -s $(TX_BASEDIR)/dxt.c dxt.c

dxa_if.h:
	ln -f -s $(TX_BASEDIR)/dxa_if.h dxa_if.h

dxp_if.h:
	ln -f -s $(TX_BASEDIR)/dxp_if.h dxp_if.h

tx_shared.h:
	ln -f -s $(TX_BASEDIR)/tx_shared.h tx_shared.h

dxt_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxt_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
