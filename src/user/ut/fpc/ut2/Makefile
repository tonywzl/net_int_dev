.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c fpc.c ut_main.c allocator.c lck.c node.c fpn.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c fpc_if.h fpc.c allocator_if.h allocator.c lck_if.h lck.c list.h node_if.h node.c fpn_if.h fpn.c list.h nid.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: fpc_unit1

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

fpn.c:
	ln -f -s $(BASEDIR)/mm/fpn.c fpn.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c
	
fpc_unit1: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f fpc_unit1 *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
