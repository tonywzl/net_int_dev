.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsrec.c ut_main.c list_sort.c cdn.c node.c blksn.c fpc.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c rtree_if.h \
	node_if.h cdn_if.h cdn.c blksn_if.h list_sort.h list_sort.c \
	dsbmp.c dsbmp_if.h rc_if.h node_if.h node.c allocator_if.h\
	dsrec_if.h dsrec.c crc_if.h crc.c nid_shared.h nid.h srn_if.h \
	blksn.c dsmgr_if.h nid.h fpc_if.h fpn_if.h fpc.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dsrec_unit

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

cdn_if.h:
	ln -f -s $(BASEDIR)/mm/cdn_if.h cdn_if.h

cdn.c:
	ln -f -s $(BASEDIR)/mm/cdn.c cdn.c

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

rc.c:
	ln -f -s $(BASEDIR)/rc.c rc.c

crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h

crc.c:
	ln -f -s $(BASEDIR)/crc.c crc.c

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

blksn.c:
	ln -f -s $(BASEDIR)/mm/blksn.c blksn.c

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h

dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c

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
