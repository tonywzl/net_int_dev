.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb   -DDEBUG_LEVEL=4

OBJS = $(SRCS:.c=.o)

BASEDIR = ../..

all: nidmanager

include manager_src.inc

nidmanager: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nidmanager *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
