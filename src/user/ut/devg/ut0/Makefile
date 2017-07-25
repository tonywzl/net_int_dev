.KEEP_STATE:

INCLUDES = -I./
LIBPATH =
SYSLIBS = -lpthread

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7

SRCS = nid_log.c dev.c devg.c rgdev.c rgdevg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h nid_shared.h \
	nid_log.h nid_log.c list.h \
	ini_if.h \
	dev_if.h dev.c \
	rgdev_if.h rgdev.c \
	rgdevg_if.h rgdevg.c \
	devg_if.h devg.c \

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
DEV_BASEDIR = ../../../dev

all: devg_unit

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

dev_if.h:
	ln -f -s $(DEV_BASEDIR)/dev_if.h dev_if.h

dev.c:
	ln -f -s $(DEV_BASEDIR)/dev.c dev.c

rgdev_if.h:
	ln -f -s $(DEV_BASEDIR)/rgdev_if.h rgdev_if.h

rgdev.c:
	ln -f -s $(DEV_BASEDIR)/rgdev.c rgdev.c

rgdevg_if.h:
	ln -f -s $(DEV_BASEDIR)/rgdevg_if.h rgdevg_if.h

rgdevg.c:
	ln -f -s $(DEV_BASEDIR)/rgdevg.c rgdevg.c

devg_if.h:
	ln -f -s $(DEV_BASEDIR)/devg_if.h devg_if.h

devg.c:
	ln -f -s $(DEV_BASEDIR)/devg.c devg.c
	

devg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f devg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
