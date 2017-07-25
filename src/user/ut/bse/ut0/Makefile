.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bse.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c lck_if.h nid.h list.h allocator_if.h node_if.h tp_if.h rtree_if.h \
	pp_if.h bsn_if.h ddn_if.h btn_if.h srn_if.h bfe_if.h sac_if.h io_if.h \
	bse_if.h bse.c

BASEDIR = ../../..

all: bse_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

btn_if.h:
	ln -f -s $(BASEDIR)/mm/btn_if.h btn_if.h

bsn_if.h:
	ln -f -s $(BASEDIR)/mm/bsn_if.h bsn_if.h

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

bfe_if.h:
	ln -f -s $(BASEDIR)/bfe_if.h bfe_if.h

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h

bse.c:
	ln -f -s $(BASEDIR)/bse.c bse.c
	

bse_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bse_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
