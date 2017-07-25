.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c crc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid_shared.h nid.h allocator_if.h fpc_if.h \
	node_if.h srn_if.h fpn_if.h cdn_if.h blksn_if.h \
	sac_if.h tp_if.h io_if.h rw_if.h rc_if.h nse_if.h cse_if.h dsmgr_if.h dsrec_if.h \
	crc_if.h crc.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: crc_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

cdn_if.h:
	ln -f -s $(BASEDIR)/mm/cdn_if.h cdn_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

nse_if.h:
	ln -f -s $(BASEDIR)/nse_if.h nse_if.h

cse_if.h:
	ln -f -s $(BASEDIR)/cse_if.h cse_if.h

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsrec_if.h:
	ln -f -s $(BASEDIR)/dsrec_if.h dsrec_if.h

crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h

crc.c:
	ln -f -s $(BASEDIR)/crc.c crc.c
	

crc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f crc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
