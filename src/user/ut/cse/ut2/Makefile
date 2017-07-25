.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=4
SRCS =  nid_log.c rc.c ut_main.c crc.c allocator.c fpn.c pp.c brn.c srn.c node.c lck.c cdn.c \
	blksn.c nse.c cse.c tp.c rw.c mrw.c drw.c dsmgr.c dsrec.c fpc.c rtree.c lstn.c list_sort.c \
	bit.c rtn.c dsbmp.c mrw_rn.c mrw_wn.c mrw_wsn.c umpk_crc.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h crc_if.h crc.c \
	rc_if.h rc.c allocator_if.h allocator.c brn_if.h brn.c fpn_if.h fpn.c pp_if.h pp.c \
	srn.c srn_if.h node.c node_if.h lck.c lck_if.h nid_shared.h nid.h cdn_if.h cdn.c \
	blksn_if.h blksn.c nse_if.h nse.c cse_if.h cse.c tp_if.h tp.c rw_if.h mrw_if.h \
	rw.c mrw.c drw_if.h drw.c dsmgr_if.h dsmgr.c dsrec_if.h dsrec.c fpc_if.h fpc.c rtree_if.h \
	rtree.c lstn_if.h lstn.c dsbmp_if.h dsbmp.c list_sort.h list_sort.c bit_if.h bit.c \
	rtn_if.h rtn.c sac_if.h io_if.h atomic.h mrw_rn_if.h mrw_rn.c mrw_wn_if.h mrw_wn.c \
	mrw_wsn_if.h mrw_wsn.c umpk_crc_if.h umpk_crc.c umpk_if.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: cse_unit2

umpk_crc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_crc_if.h umpk_crc_if.h

umpk_crc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_crc.c umpk_crc.c

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_if.h umpk_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

rtn_if.h:
	ln -f -s $(BASEDIR)/mm/rtn_if.h rtn_if.h

mrw_rn_if.h:
	ln -f -s $(BASEDIR)/mm/mrw_rn_if.h mrw_rn_if.h

mrw_rn.c:
	ln -f -s $(BASEDIR)/mm/mrw_rn.c mrw_rn.c

mrw_wn_if.h:
	ln -f -s $(BASEDIR)/mm/mrw_wn_if.h mrw_wn_if.h

mrw_wn.c:
	ln -f -s $(BASEDIR)/mm/mrw_wn.c mrw_wn.c

mrw_wsn_if.h:
	ln -f -s $(BASEDIR)/mm/mrw_wsn_if.h mrw_wsn_if.h

mrw_wsn.c:
	ln -f -s $(BASEDIR)/mm/mrw_wsn.c mrw_wsn.c
	
rtn.c:
	ln -f -s $(BASEDIR)/mm/rtn.c rtn.c

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h
	
bit.c:
	ln -f -s $(BASEDIR)/bit.c bit.c

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c

dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h
	
dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h
	
lstn.c:
	ln -f -s $(BASEDIR)/mm/lstn.c lstn.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h
	
rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h
	
fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

dsrec_if.h:
	ln -f -s $(BASEDIR)/dsrec_if.h dsrec_if.h
	
dsrec.c:
	ln -f -s $(BASEDIR)/dsrec.c dsrec.c

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h
	
dsmgr.c:
	ln -f -s $(BASEDIR)/dsmgr.c dsmgr.c

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h
	
rw.c:
	ln -f -s $(BASEDIR)/rw.c rw.c

mrw_if.h:
	ln -f -s $(BASEDIR)/mrw_if.h mrw_if.h
	
mrw.c:
	ln -f -s $(BASEDIR)/mrw.c mrw.c

drw_if.h:
	ln -f -s $(BASEDIR)/drw_if.h drw_if.h
	
drw.c:
	ln -f -s $(BASEDIR)/drw.c drw.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

cse_if.h:
	ln -f -s $(BASEDIR)/cse_if.h cse_if.h
	
cse.c:
	ln -f -s $(BASEDIR)/cse.c cse.c

nse_if.h:
	ln -f -s $(BASEDIR)/nse_if.h nse_if.h
	
nse.c:
	ln -f -s $(BASEDIR)/nse.c nse.c

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h
	
blksn.c:
	ln -f -s $(BASEDIR)/mm/blksn.c blksn.c

cdn_if.h:
	ln -f -s $(BASEDIR)/mm/cdn_if.h cdn_if.h
	
cdn.c:
	ln -f -s $(BASEDIR)/mm/cdn.c cdn.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h
	
lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c
	
node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h
	
node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h
	
srn.c:
	ln -f -s $(BASEDIR)/mm/srn.c srn.c

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h
	
pp.c:
	ln -f -s $(BASEDIR)/mm/pp.c pp.c

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h
	
fpn.c:
	ln -f -s $(BASEDIR)/mm/fpn.c fpn.c

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h
	
brn.c:
	ln -f -s $(BASEDIR)/mm/brn.c brn.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h
	
allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h
	
crc.c:
	ln -f -s $(BASEDIR)/crc.c crc.c

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

rc.c:
	ln -f -s $(BASEDIR)/rc.c rc.c
	

cse_unit2: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cse_unit2 *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
