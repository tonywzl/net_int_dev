.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  sah.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h list.h nid_log.h ini_if.h \
	    sah_if.h sah.c

all: sah_unit
	
nid.h:
	ln -f -s ../../../nid.h nid.h

list.h:
	ln -f -s ../../../list.h list.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h

sah_if.h:
	ln -f -s ../../../sah_if.h sah_if.h

sah.c:
	ln -f -s ../../../sah.c sah.c

	

sah_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sah_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
