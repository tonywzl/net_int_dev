.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c allocator.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h lck_if.h \
	allocator_if.h allocator.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: allocator_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

allocator_if.h:
	ln -f -s $(BASEDIR)/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/allocator.c allocator.c
	

allocator_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f allocator_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
