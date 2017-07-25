.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c crcg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c nid_shared.h list.h ini_if.h \
	ppg_if.h \
	rc_if.h crc_if.h crcg_if.h crcg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared
MM_BASEDIR = ../../../mm

all: crcg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ppg_if.h:
	ln -f -s $(BASEDIR)/ppg_if.h ppg_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h

crcg_if.h:
	ln -f -s $(BASEDIR)/crcg_if.h crcg_if.h

crcg.c:
	ln -f -s $(BASEDIR)/crcg.c crcg.c
	

crcg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f crcg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
