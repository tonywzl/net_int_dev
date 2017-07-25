.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c fpc.c ut_main.c allocator.c lck.c node.c fpn.c server_conf.c hash.c ini.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c fpc_if.h fpc.c allocator_if.h allocator.c lck_if.h lck.c \
	   ini_if.h hash_if.h nid.h  nid_log.c hash.c list.h ini_if.h ini.c server.h server_conf.c \
	   node_if.h node.c fpn_if.h fpn.c list.h

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: hash_fpc

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

fpn.c:
	ln -f -s $(BASEDIR)/mm/fpn.c fpn.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

hash_if.h:
	ln -f -s $(BASEDIR)/hash_if.h hash_if.h	

hash.c:
	ln -f -s $(BASEDIR)/hash.c hash.c

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

server_conf.c:
	ln -f -s $(BASEDIR)/server_conf.c server_conf.c

	
hash_fpc: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS) $(LIBPATH)

tags:
	ctags *.[ch] 

clean:
	rm -f hash_fpc *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
