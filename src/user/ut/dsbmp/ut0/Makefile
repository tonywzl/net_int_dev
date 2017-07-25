.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dsbmp.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c \
	btn_if.h rtree_if.h node_if.h brn_if.h blksn_if.h dsmgr_if.h \
	dsbmp_if.h dsbmp.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: dsbmp_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

btn_if.h:
	ln -f -s $(BASEDIR)/mm/btn_if.h btn_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

blksn_if.h:
	ln -f -s $(BASEDIR)/mm/blksn_if.h blksn_if.h

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h

dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h

dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c
	

dsbmp_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsbmp_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
