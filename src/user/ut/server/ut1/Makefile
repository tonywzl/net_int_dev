.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  tp.c hs.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h list.h nid_log.h \
	ini_if.h tp_if.h tp.c hs_if.h hs.c

all: ut_client 
	
nid.h:
	ln -f -s ../../../nid.h nid.h
list.h:
	ln -f -s ../../../list.h list.h
nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

tp_if.h:
	ln -f -s ../../../tp_if.h tp_if.h
	
tp.c:
	ln -f -s ../../../tp.c tp.c

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h
	
hs_if.h:
	ln -f -s ../../../hs_if.h hs_if.h
	
hs.c:
	ln -f -s ../../../hs.c hs.c

ut_client: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut_client *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
