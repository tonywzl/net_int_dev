.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  mq.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid.h \
	mq_if.h mq.c

BASEDIR = ../../..
all: mq_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

mq.c:
	ln -f -s $(BASEDIR)/mq.c mq.c

mq_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f mq_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
