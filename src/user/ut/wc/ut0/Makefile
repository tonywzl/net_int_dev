.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c wc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_shared.h nid_log.h nid_log.c list.h allocator_if.h \
	bwc_if.h twc_if.h \
	io_if.h \
	wc_if.h wc.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
MM_BASEDIR = ../../../mm
CACHE_BASEDIR = ../../../cache

all: wc_unit
	
nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

bwc_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwc_if.h bwc_if.h

twc_if.h:
	ln -f -s $(CACHE_BASEDIR)/twc_if.h twc_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h

wc.c:
	ln -f -s $(CACHE_BASEDIR)/wc.c wc.c
	

wc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f wc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
