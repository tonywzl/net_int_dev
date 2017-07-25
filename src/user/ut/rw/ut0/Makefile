.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rw.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c drw_if.h mrw_if.h \
	node_if.h \
	rw_if.h rw.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: rw_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

drw_if.h:
	ln -f -s $(BASEDIR)/drw_if.h drw_if.h

mrw_if.h:
	ln -f -s $(BASEDIR)/mrw_if.h mrw_if.h

node_if.h:
	ln -f -s $(BASEDIR)/node_if.h node_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

rw.c:
	ln -f -s $(BASEDIR)/rw.c rw.c
	

rw_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rw_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
