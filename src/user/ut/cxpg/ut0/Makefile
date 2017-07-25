.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c cxpg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h nid.h nid_log.h nid_log.c nid_shared.h list.h ini_if.h \
	allocator_if.h node_if.h lstn_if.h \
	cxp_if.h \
	cxpg_if.h cxpg.c

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: cxpg_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

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

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

cxp_if.h:
	ln -f -s $(DX_BASEDIR)/cxp_if.h cxp_if.h

cxpg_if.h:
	ln -f -s $(DX_BASEDIR)/cxpg_if.h cxpg_if.h

cxpg.c:
	ln -f -s $(DX_BASEDIR)/cxpg.c cxpg.c
	

cxpg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cxpg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
