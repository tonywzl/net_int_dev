.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsrec.c list_sort.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c rtree_if.h \
	node_if.h cdn_if.h blksn_if.h \
	dsmgr_if.h rc_if.h list_sort.h list_sort.c \
	dsrec_if.h dsrec.c nid.h fpc_if.h fpn_if.h \
	node_if.h allocator_if.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dsrec_unit

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c
	
allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

cdn_if.h:
	ln -f -s $(BASEDIR)/mm/cdn_if.h cdn_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsrec_if.h:
	ln -f -s $(BASEDIR)/dsrec_if.h dsrec_if.h

dsrec.c:
	ln -f -s $(BASEDIR)/dsrec.c dsrec.c

dsrec_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsrec_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
