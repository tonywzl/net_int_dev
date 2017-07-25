.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c fpc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c fpc_if.h fpc.c list.h fpn_if.h node_if.h nid.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: fpc_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
	
fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h
	
node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h
	
fpc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f fpc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
