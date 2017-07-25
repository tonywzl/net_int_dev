.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c nse.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h lck_if.h \
	node_if.h srn_if.h brn_if.h fpn_if.h brn_if.h \
	rtree_if.h \
	nse_if.h nse.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: nse_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h

nse_if.h:
	ln -f -s $(BASEDIR)/nse_if.h nse_if.h

nse.c:
	ln -f -s $(BASEDIR)/nse.c nse.c
	

nse_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nse_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
