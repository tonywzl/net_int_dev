.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rsun.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h \
	allocator_if.h node_if.h \
	rsun_if.h rsun.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
MM_BASEDIR = ../../../mm

all: rsun_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

rsun_if.h:
	ln -f -s $(MM_BASEDIR)/rsun_if.h rsun_if.h

rsun.c:
	ln -f -s $(MM_BASEDIR)/rsun.c rsun.c
	

rsun_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rsun_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
