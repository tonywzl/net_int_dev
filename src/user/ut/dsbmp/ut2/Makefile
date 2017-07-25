.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsbmp.c ut_main.c blksn.c node.c brn.c rtree.c rtn.c ddn.c btn.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c node.c allocator_if.h \
	rtree_if.h ddn_if.h btn_if.h rtn_if.h bit_if.h blksn_if.h brn_if.h node_if.h \
	dsbmp_if.h dsbmp.c blksn.c dsmgr_if.h dsmgr.c ddn.c btn.c rtn.c blksn.c brn.c 

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared




all: dsbmp_unit

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h


dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

blksn.c:
	ln -f -s $(BASEDIR)/mm/blksn.c blksn.c

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsmgr.c:
	ln -f -s $(BASEDIR)/dsmgr.c dsmgr.c

rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

brn.c:
	ln -f -s $(BASEDIR)/mm/brn.c brn.c

dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

btn_if.h:
	ln -f -s $(BASEDIR)/mm/btn_if.h btn_if.h

btn.c:
	ln -f -s $(BASEDIR)/mm/btn.c btn.c

ddn.c:
	ln -f -s $(BASEDIR)/mm/ddn.c ddn.c

rtn_if.h:
	ln -f -s $(BASEDIR)/mm/rtn_if.h rtn_if.h

btn.c:
	ln -f -s $(BASEDIR)/mm/btn.c btn.c

rtn.c:
	ln -f -s $(BASEDIR)/mm/rtn.c rtn.c

ddn.c:
	ln -f -s $(BASEDIR)/mm/ddn.c ddn.c

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

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
