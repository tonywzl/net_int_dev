.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bre.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h lck_if.h node_if.h \
	ddn_if.h pp_if.h bse_if.h \
	bre_if.h bre.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: bre_unit
	
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

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h

bre_if.h:
	ln -f -s $(BASEDIR)/bre_if.h bre_if.h

bre.c:
	ln -f -s $(BASEDIR)/bre.c bre.c
	

bre_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bre_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
