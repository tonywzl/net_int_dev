.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bfe0.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_shared.h nid_log.h nid_log.c lck_if.h atomic.h allocator_if.h \
	pp_if.h node_if.h lstn_if.h d1an_if.h d2cn_if.h fpn_if.h ddn_if.h bre_if.h bse_if.h wc_if.h rc_if.h io_if.h rw_if.h \
	bfe0_if.h bfe0.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
CACHE_BASEDIR = ../../../cache
RW_BASEDIR = ../../../rw
MM_BASEDIR = ../../../mm

all: bfe0_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

d1an_if.h:
	ln -f -s $(MM_BASEDIR)/d1an_if.h d1an_if.h

d2cn_if.h:
	ln -f -s $(MM_BASEDIR)/d2cn_if.h d2cn_if.h

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h

ddn_if.h:
	ln -f -s $(MM_BASEDIR)/ddn_if.h ddn_if.h

bre_if.h:
	ln -f -s $(CACHE_BASEDIR)/bre_if.h bre_if.h

bse_if.h:
	ln -f -s $(CACHE_BASEDIR)/bse_if.h bse_if.h

wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h

rc_if.h:
	ln -f -s $(CACHE_BASEDIR)/rc_if.h rc_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

rw_if.h:
	ln -f -s $(RW_BASEDIR)/rw_if.h rw_if.h

bfe0_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfe0_if.h bfe0_if.h

bfe0.c:
	ln -f -s $(CACHE_BASEDIR)/bfe0.c bfe0.c
	
bfe0_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bfe0_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
