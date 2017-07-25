.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  msc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS =  nid_log.h nid.h nid_shared.h mpk_if.h\
	    msc_if.h msc.c

BASEDIR = ../../..
all: msc_unit
	
nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid.h:
	ln -f -s ../../../nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

msc_if.h:
	ln -f -s ../../../msc_if.h msc_if.h

msc.c:
	ln -f -s ../../../msc.c msc.c

msc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f msc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
