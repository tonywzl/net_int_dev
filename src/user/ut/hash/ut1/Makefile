.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBPATH = -lglib-2.0

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c hash.c ini.c server_conf.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h ini_if.h hash_if.h nid.h  nid_log.c hash.c list.h ini.c server.h server_conf.c

BASEDIR = ../../..

all: hash_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

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

hash_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH)

tags:
	ctags *.[ch] 

clean:
	rm -f hash_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
