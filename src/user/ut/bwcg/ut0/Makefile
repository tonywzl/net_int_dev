.KEEP_STATE:

BASEDIR = ../../../
UMPK_BASEDIR = ../../../umpk
MM_BASEDIR = ../../../mm
DX_BASEDIR = ../../../dx
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wcast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=4

OBJS = $(SRCS:.c=.o)

all: bwcg_unit

SRCS = ut_main.c

include server_src.inc


bwcg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bwcg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
