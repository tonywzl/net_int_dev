.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rccg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid.h \
	node_if.h lstn_if.h \
	scg_if.h rcc_if.h rc_if.h \
	rccg_if.h rccg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: rccg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

rcc_if.h:
	ln -f -s $(BASEDIR)/rcc_if.h rcc_if.h

rccg_if.h:
	ln -f -s $(BASEDIR)/rccg_if.h rccg_if.h

rccg.c:
	ln -f -s $(BASEDIR)/rccg.c rccg.c
	

rccg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rccg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
