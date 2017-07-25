.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c cse.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h lck_if.h atomic.h fpc_if.h allocator_if.h \
	node_if.h pp_if.h srn_if.h brn_if.h fpn_if.h brn_if.h cdn_if.h blksn_if.h \
	rc_if.h \
	cse_if.h cse.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: cse_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h 

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

cdn_if.h:
	ln -f -s $(BASEDIR)/mm/cdn_if.h cdn_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

cse_if.h:
	ln -f -s $(BASEDIR)/cse_if.h cse_if.h

cse.c:
	ln -f -s $(BASEDIR)/cse.c cse.c
	

cse_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cse_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
