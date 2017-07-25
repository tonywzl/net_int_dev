.KEEP_STATE:

BASEDIR = ../../../
INCLUDES = -I./ -I$(BASEDIR)/../../build/ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = util_nw.c asc.c asc-sm.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = version.h list.h nid_log.h tp_if.h nid.h nid_shared.h \
	   util_nw.h util_nw.c \
	   dsm_if.h \
	   mpk_if.h \
	   adt_if.h \
	   nl_if.h \
	   ash_if.h \
	   acg_if.h \
	   asc_if.h asc.h asc.c asc-sm.c

all: asc_unit
	
version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

dsm_if.h:
	ln -f -s $(BASEDIR)/dsm_if.h dsm_if.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

adt_if.h:
	ln -f -s $(BASEDIR)/adt_if.h adt_if.h

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

ash_if.h:
	ln -f -s $(BASEDIR)/ash_if.h ash_if.h

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

asc.h:
	ln -f -s $(BASEDIR)/asc.h asc.h

asc.c:
	ln -f -s $(BASEDIR)/asc.c asc.c

asc-sm.c:
	ln -f -s $(BASEDIR)/asc-sm.c asc-sm.c

asc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f asc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
