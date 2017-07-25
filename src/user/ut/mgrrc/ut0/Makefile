.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = util_nw.c nid_log.c mgrrc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS =  util_nw.h util_nw.c nid_log.h nid_log.c nid.h nid_shared.h \
	    umpk_if.h umpk_rc_if.h \
	    mgrrc_if.h mgrrc.c

BASEDIR = ../../..
all: mgrrc_unit
	

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_log.c:
	ln -f -s ../../../nid_log.c nid_log.c

nid.h:
	ln -f -s ../../../nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk_if.h umpk_if.h

umpk_rc_if.h:
	ln -f -s $(BASEDIR)/umpk_rc_if.h umpk_rc_if.h

mgrrc_if.h:
	ln -f -s ../../../mgrrc_if.h mgrrc_if.h

mgrrc.c:
	ln -f -s ../../../mgrrc.c mgrrc.c

mgrrc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f mgrrc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
