.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsmgr.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c allocator_if.h umpk_crc_if.h nid.h \
	node_if.h brn_if.h blksn_if.h rtree_if.h dsbmp_if.h nid_shared.h \
	dsmgr_if.h dsmgr.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dsmgr_unit


umpk_crc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_crc_if.h umpk_crc_if.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsmgr.c:
	ln -f -s $(BASEDIR)/dsmgr.c dsmgr.c

dsmgr_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsmgr_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
