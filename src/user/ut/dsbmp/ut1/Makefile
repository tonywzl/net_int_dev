.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsbmp.c blksn.c brn.c ut_main.c node.c rtree.c rtn.c bit.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c allocator_if.h blksn_if.h \
	rtree_if.h node_if.h brn_if.h dsmgr_if.h blksn.c brn.c \
	dsbmp_if.h dsbmp.c bit_if.h rtn_if.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared




all: dsbmp_unit

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h

bit.c:
	ln -f -s $(BASEDIR)/bit.c bit.c

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

rtn_if.h:
	ln -f -s $(BASEDIR)/mm/rtn_if.h rtn_if.h

rtn.c:
	ln -f -s $(BASEDIR)/mm/rtn.c rtn.c

brn.c:
	ln -f -s $(BASEDIR)/mm/brn.c brn.c

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

blksn.c:
	ln -f -s $(BASEDIR)/mm/blksn.c blksn.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

sdn_if.h:
	ln -f -s $(BASEDIR)/mm/sdn_if.h sdn_if.h

dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h

dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c

#ut_main.c:
#	ut_main.c

dsbmp_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsbmp_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
