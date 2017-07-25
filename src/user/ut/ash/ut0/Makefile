.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  ash.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h list.h nid_log.h ini_if.h \
	    ash_if.h ash.c

all: ash_unit
	
nid.h:
	ln -f -s ../../../nid.h nid.h

list.h:
	ln -f -s ../../../list.h list.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h

ash_if.h:
	ln -f -s ../../../ash_if.h ash_if.h

ash.c:
	ln -f -s ../../../ash.c ash.c

	

ash_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ash_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
