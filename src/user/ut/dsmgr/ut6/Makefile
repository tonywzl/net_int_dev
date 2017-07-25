.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  rtn.c node.c rtree.c dsbmp.c brn.c blksn.c nid_log.c dsmgr.c allocator.c lck.c util_list.c umpk_rc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c node.c allocator_if.h \
	node_if.h brn_if.h blksn_if.h rtree_if.h dsbmp_if.h bit_if.h  ddn_if.h btn_if.h rtn_if.h \
	dsmgr_if.h dsmgr.c ddn.c btn.c rtn.c allocator_if.h allocator.c blksn.c brn.c dsbmp.c rtree.c \
	umpk_crc_if.h nid.h nid_shared.h lck_if.h lck.c util_list.h util_list.c umpk_rc_if.h umpk_rc.c \
	umpk_if.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dsmgr_unit

umpk_crc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_crc_if.h umpk_crc_if.h
	
umpk_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_if.h umpk_if.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
	
lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c
	
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

util_list.h:
	ln -f -s $(BASEDIR)/util_list.h util_list.h

util_list.c:
	ln -f -s $(BASEDIR)/util_list.c util_list.c

umpk_rc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_rc_if.h umpk_rc_if.h

umpk_rc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_rc.c umpk_rc.c

rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

blksn.c:
	ln -f -s $(BASEDIR)/mm/blksn.c blksn.c

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

dsmgr_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsmgr_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
