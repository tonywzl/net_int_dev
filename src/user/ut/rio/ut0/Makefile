.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rio.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.c nid_log.h nid_shared.h lck_if.h \
	pp_if.h rw_if.h io_if.h \
	rio_if.h rio.c

BASEDIR = ../../..

all: rio_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/pp_if.h pp_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h

rio.c:
	ln -f -s $(BASEDIR)/rio.c rio.c
	

rio_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rio_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
