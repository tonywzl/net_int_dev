.KEEP_STATE:

BASEDIR = ../../..
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wcast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
CFLAGS += -DMETASERVER 

OBJS = $(SRCS:.c=.o)

all: ut_main

SRCS = ut_main.c ms.c

SYMLINKS = 

include ../../../build/mds/mds_src.inc
include ../../../build/server/server_src.inc

ut_main: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut_main *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)