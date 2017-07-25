.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dxacg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_shared.h nid_log.h nid_log.c list.h nid.h \
	node_if.h lstn_if.h \
	scg_if.h dxac_if.h dxa_if.h \
	dxacg_if.h dxacg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dxacg_unit

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

dxac_if.h:
	ln -f -s $(BASEDIR)/dxac_if.h dxac_if.h

dxa_if.h:
	ln -f -s $(BASEDIR)/dxa_if.h dxa_if.h

dxacg_if.h:
	ln -f -s $(BASEDIR)/dxacg_if.h dxacg_if.h

dxacg.c:
	ln -f -s $(BASEDIR)/dxacg.c dxacg.c
	

dxacg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxacg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
