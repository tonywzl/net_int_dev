.KEEP_STATE:

INCLUDES = -I./ 
LIppATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIppATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  lck.c allocator.c pp2.c tp.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_shared.h nid.h nid_log.h list.h lck_if.h lck.c allocator_if.h allocator.c pp2_if.h pp2.c tp_if.h tp.c

BASEDIR = ../../../..

all: pp2_unit

nid_shared.h:
	ln -f -s $(BASEDIR)/shared/nid_shared.h nid_shared.h
	
list.h:
	ln -f -s $(BASEDIR)/user/list.h list.h
	
nid.h:
	ln -f -s $(BASEDIR)/user/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/user/nid_log.h nid_log.h

lck_if.h:
	ln -f -s $(BASEDIR)/user/lck_if.h lck_if.h
	
lck.c:
	ln -f -s $(BASEDIR)/user/lck.c lck.c

tp_if.h:
	ln -f -s $(BASEDIR)/user/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/user/tp.c tp.c

allocator_if.h:
	ln -f -s $(BASEDIR)/user/mm/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/user/mm/allocator.c allocator.c

pp2_if.h:
	ln -f -s $(BASEDIR)/user/mm/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(BASEDIR)/user/mm/pp2.c pp2.c
	

pp2_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f pp2_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
