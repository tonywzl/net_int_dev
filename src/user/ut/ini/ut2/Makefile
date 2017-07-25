.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include 
LIBPATH = -lglib-2.0 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  ini.c ut_main.c nid_log.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h ini_if.h ini.c nid_log.h nid_log.c

all: ini_unit
	
list.h:
	ln -f -s ../../../list.h list.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_log.c:
	ln -f -s ../../../nid_log.c nid_log.c

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h

ini.c:
	ln -f -s ../../../ini.c ini.c

	

ini_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ini_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
