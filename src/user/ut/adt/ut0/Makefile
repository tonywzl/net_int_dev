.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  adt.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid.h nl_if.h mq_if.h \
	adt_if.h adt.c

BASEDIR = ../../..
all: adt_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

adt_if.h:
	ln -f -s $(BASEDIR)/adt_if.h adt_if.h

adt.c:
	ln -f -s $(BASEDIR)/adt.c adt.c

adt_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f adt_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
