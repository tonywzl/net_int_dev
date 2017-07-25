.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = util_nw.c nid_log.c rcc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c list.h tp_if.h nid_log.c nid_log.h nid_shared.h nid.h \
	umpk_if.h umpk_rc_if.h io_if.h bio_if.h \
	rc_if.h rccg_if.h sac_if.h \
	rcc_if.h rcc.c

BASEDIR = ../../..
BASESHAREDDIR = ../../../../shared

all: rcc_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASESHAREDDIR)/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk_if.h umpk_if.h

umpk_rc_if.h:
	ln -f -s $(BASEDIR)/umpk_rc_if.h umpk_rc_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

rccg_if.h:
	ln -f -s $(BASEDIR)/rccg_if.h rccg_if.h

rcc_if.h:
	ln -f -s $(BASEDIR)/rcc_if.h rcc_if.h

rcc.c:
	ln -f -s $(BASEDIR)/rcc.c rcc.c

rcc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rcc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
