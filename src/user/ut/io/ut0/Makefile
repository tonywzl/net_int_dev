.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c io.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h tp_if.h \
	rio_if.h bio_if.h \
	io_if.h io.c

BASEDIR = ../../..

all: io_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

io.c:
	ln -f -s $(BASEDIR)/io.c io.c
	

io_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f io_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
