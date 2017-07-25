.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c drw.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c \
	node_if.h fpn_if.h \
	rw_if.h fpc_if.h \
	drw_if.h drw.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: drw_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

drw_if.h:
	ln -f -s $(BASEDIR)/drw_if.h drw_if.h

drw.c:
	ln -f -s $(BASEDIR)/drw.c drw.c
	

drw_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f drw_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
