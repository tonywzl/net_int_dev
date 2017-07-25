.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c wccg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid.h \
	node_if.h lstn_if.h \
	scg_if.h wcc_if.h wc_if.h \
	wccg_if.h wccg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: wccg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(BASEDIR)/lstn_if.h lstn_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

wcc_if.h:
	ln -f -s $(BASEDIR)/wcc_if.h wcc_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

wccg_if.h:
	ln -f -s $(BASEDIR)/wccg_if.h wccg_if.h

wccg.c:
	ln -f -s $(BASEDIR)/wccg.c wccg.c
	

wccg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f wccg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
