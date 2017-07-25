.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = nid_log.c umpk.c umpk_wc.c umpk_rc.c ut_main.c

OBJS = $(SRCS:.c=.o)
SYMLINKS = nid.h nid_log.h nid_log.c nid_shared.h \
	umpk_if.h umpk.h umpk.c \
	umpk_wc_if.h umpk_wc.c \
	umpk_rc_if.h umpk_rc.c

BASEDIR = ../../..

all: umpk_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_if.h umpk_if.h
	
umpk.h:
	ln -f -s $(BASEDIR)/umpk/umpk.h umpk.h
	
umpk.c:
	ln -f -s $(BASEDIR)/umpk/umpk.c umpk.c
	
umpk_wc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_wc_if.h umpk_wc_if.h

umpk_wc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_wc.c umpk_wc.c
	
umpk_rc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_rc_if.h umpk_rc_if.h

umpk_rc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_rc.c umpk_rc.c
	
umpk_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f umpk_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
