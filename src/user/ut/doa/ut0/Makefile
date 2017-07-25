.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c doa.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid_shared.h mpk_if.h \
	scg_if.h \
	doa_if.h doa.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: doa_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(SHARED_BASEDIR)/mpk_if.h mpk_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

doa_if.h:
	ln -f -s $(BASEDIR)/doa_if.h doa_if.h

doa.c:
	ln -f -s $(BASEDIR)/doa.c doa.c
	

doa_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f doa_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
