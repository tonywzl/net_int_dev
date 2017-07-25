.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bwccg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid.h \
	node_if.h lstn_if.h \
	scg_if.h bwcc_if.h bwc_if.h \
	bwccg_if.h bwccg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: bwccg_unit
	
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

bwcc_if.h:
	ln -f -s $(BASEDIR)/bwcc_if.h bwcc_if.h

bwc_if.h:
	ln -f -s $(BASEDIR)/bwc_if.h bwc_if.h

bwccg_if.h:
	ln -f -s $(BASEDIR)/bwccg_if.h bwccg_if.h

bwccg.c:
	ln -f -s $(BASEDIR)/bwccg.c bwccg.c
	

bwccg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bwccg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
