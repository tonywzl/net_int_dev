.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c nid_log.c cxt.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c nid_log.h nid_log.c nid_shared.h list.h \
	node_if.h lstn_if.h txn_if.h pp2_if.h \
	ds_if.h \
	cxt_if.h cxt.c

BASEDIR = ../../..
TX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: cxt_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

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

cxt_if.h:
	ln -f -s $(TX_BASEDIR)/cxt_if.h cxt_if.h

cxt.c:
	ln -f -s $(TX_BASEDIR)/cxt.c cxt.c

cxt_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cxt_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
