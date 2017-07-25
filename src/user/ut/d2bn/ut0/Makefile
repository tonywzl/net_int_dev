.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c d2bn.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h node_if.h \
	rw_if.h \
	d2bn_if.h d2bn.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: d2bn_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

d2bn_if.h:
	ln -f -s $(BASEDIR)/mm/d2bn_if.h d2bn_if.h

d2bn.c:
	ln -f -s $(BASEDIR)/mm/d2bn.c d2bn.c
	

d2bn_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f d2bn_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
