.KEEP_STATE:

INCLUDES = -I./ 
LIppATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIppATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  pp.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h list.h lck_if.h allocator_if.h \
	pp_if.h pp.c

BASEDIR = ../../..

all: pp_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

pp.c:
	ln -f -s $(BASEDIR)/mm/pp.c pp.c
	

pp_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f pp_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
