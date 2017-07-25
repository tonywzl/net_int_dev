.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c ds.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c nid.h nid_shared.h \
	   cds_if.h sds_if.h \
	   ds_if.h ds.c

all: ds_unit
	
list.h:
	ln -f -s ../../../list.h list.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_log.c:
	ln -f -s ../../../nid_log.c nid_log.c

nid_shared.h:
	ln -f -s ../../../../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s ../../../nid.h nid.h

cds_if.h:
	ln -f -s ../../../cds_if.h cds_if.h

sds_if.h:
	ln -f -s ../../../sds_if.h sds_if.h

ds_if.h:
	ln -f -s ../../../ds_if.h ds_if.h

ds.c:
	ln -f -s ../../../ds.c ds.c

ds_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ds_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
