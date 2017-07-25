.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  tp.c ut_main.c nid_log.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c tp_if.h tp.c 
BASEDIR = ../../..

all: tp_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h
nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

	

tp_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f tp_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
