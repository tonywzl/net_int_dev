.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c drwg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h lstn_if.h ini_if.h \
	node_if.h lstn_if.h \
	rw_if.h drw_if.h drwg_if.h drwg.c

BASEDIR = ../../..
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared

all: drwg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

drw_if.h:
	ln -f -s $(BASEDIR)/drw_if.h drw_if.h

drwg_if.h:
	ln -f -s $(BASEDIR)/drwg_if.h drwg_if.h

drwg.c:
	ln -f -s $(BASEDIR)/drwg.c drwg.c
	

drwg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f drwg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
