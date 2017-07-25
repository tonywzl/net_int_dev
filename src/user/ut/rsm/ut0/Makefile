.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rsm.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c nid_shared.h list.h \
	allocator_if.h lstn_if.h node_if.h rsn_if.h rsun_if.h \
	rs_if.h rsg_if.h \
	rsm_if.h rsm.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
RS_BASEDIR = ../../../rs
MM_BASEDIR = ../../../mm

all: rsm_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h 

rsun_if.h:
	ln -f -s $(MM_BASEDIR)/rsun_if.h rsun_if.h

rsn_if.h:
	ln -f -s $(MM_BASEDIR)/rsn_if.h rsn_if.h

rs_if.h:
	ln -f -s $(RS_BASEDIR)/rs_if.h rs_if.h

rsg_if.h:
	ln -f -s $(RS_BASEDIR)/rsg_if.h rsg_if.h

rsm_if.h:
	ln -f -s $(RS_BASEDIR)/rsm_if.h rsm_if.h

rsm.c:
	ln -f -s $(RS_BASEDIR)/rsm.c rsm.c
	

rsm_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rsm_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
