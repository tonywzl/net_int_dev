.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nw.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid.h nid_shared.h\
	tp_if.h scg_if.h acg_if.h \
	nw.h nw_if.h nw.c

BASEDIR = ../../..

all: nw_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h
	
acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h
	
nw.h:
	ln -f -s $(BASEDIR)/nw.h nw.h

nw_if.h:
	ln -f -s $(BASEDIR)/nw_if.h nw_if.h

nw.c:
	ln -f -s $(BASEDIR)/nw.c nw.c

nw_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nw_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
