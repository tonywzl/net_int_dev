.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7

SRCS =  nid_log.c rsg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c ini_if.h ini.c list.h\
	allocator_if.h lstn_if.h node_if.h node.c spn_if.h  rs_if.h rs.c rsun_if.h rsun.c nid.h nid_shared.h \
	rsg_if.h rsg.c 

BASEDIR = ../../..
RS_BASEDIR = ../../../rs
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: rsg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

rsg_if.h:
	ln -f -s $(RS_BASEDIR)/rsg_if.h rsg_if.h

rsg.c:
	ln -f -s $(RS_BASEDIR)/rsg.c rsg.c

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

node.c:
	ln -f -s $(MM_BASEDIR)/node.c node.c

spn_if.h:
	ln -f -s $(MM_BASEDIR)/spn_if.h spn_if.h

rs_if.h:
	ln -f -s $(RS_BASEDIR)/rs_if.h rs_if.h

rs.c:
	ln -f -s $(RS_BASEDIR)/rs.c rs.c

rsun_if.h:
	ln -f -s $(MM_BASEDIR)/rsun_if.h rsun_if.h

rsun.c:
	ln -f -s $(MM_BASEDIR)/rsun.c rsun.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

rsg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rsg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
