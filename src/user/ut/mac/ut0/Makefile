.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  mac.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS =  nid_log.h nid.h nid_shared.h \
	    mpk_if.h \
	    mac_if.h mac.c

BASEDIR = ../../..

all: mac_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mac_if.h:
	ln -f -s $(BASEDIR)/mac_if.h mac_if.h

mac.c:
	ln -f -s $(BASEDIR)/mac.c mac.c

mac_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f mac_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
