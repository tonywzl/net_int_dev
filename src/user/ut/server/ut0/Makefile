.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  tp.c sah.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h list.h nid_log.h \
	ini_if.h ini.c	\
	tp_if.h tp.c \
	sah_if.h sah.c

all: ut_client 
	
nid.h:
	ln -f -s ../../../nid.h nid.h
list.h:
	ln -f -s ../../../list.h list.h
nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h
	
ini.c:
	ln -f -s ../../../ini.c ini.c

tp_if.h:
	ln -f -s ../../../tp_if.h tp_if.h
	
tp.c:
	ln -f -s ../../../tp.c tp.c

sah_if.h:
	ln -f -s ../../../sah_if.h sah_if.h
	
sah.c:
	ln -f -s ../../../sah.c sah.c

ut_client: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut_client *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
