.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rtn.c ddn.c btn.c rtree.c node.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h bit_if.h node_if.h node.c allocator_if.h \
	rtn_if.h rtn.c \
	btn_if.h btn.c \
	ddn_if.h ddn.c \
	rtree_if.h rtree.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: rtree_unit

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h

rtn_if.h:
	ln -f -s $(BASEDIR)/mm/rtn_if.h rtn_if.h

rtn.c:
	ln -f -s $(BASEDIR)/mm/rtn.c rtn.c

btn_if.h:
	ln -f -s $(BASEDIR)/mm/btn_if.h btn_if.h

btn.c:
	ln -f -s $(BASEDIR)/mm/btn.c btn.c

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

ddn.c:
	ln -f -s $(BASEDIR)/mm/ddn.c ddn.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c

rtree_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rtree_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
