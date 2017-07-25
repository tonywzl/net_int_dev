.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c cxag.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c nid_shared.h list.h ini_if.h \
	allocator_if.h node_if.h lstn_if.h \
	cxa_if.h \
	cxag_if.h cxag.c

BASEDIR = ../../..
TX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: cxag_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

cxa_if.h:
	ln -f -s $(TX_BASEDIR)/cxa_if.h cxa_if.h

cxag_if.h:
	ln -f -s $(TX_BASEDIR)/cxag_if.h cxag_if.h

cxag.c:
	ln -f -s $(TX_BASEDIR)/cxag.c cxag.c
	

cxag_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cxag_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
