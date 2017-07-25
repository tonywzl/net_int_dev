.KEEP_STATE:

INCLUDES = -I./ 
LIppATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIppATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c cds.c ut_main.c  lck.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c nid.h nid_shared.h list.h  lck_if.h \
	io_if.h lck.c \
	cds_if.h cds.c

BASEDIR = ../../..
MM_BASEDIR = ../../../mm

all: cds_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

list.c:
	ln -f -s $(BASEDIR)/list.c list.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h


cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h

cds.c:
	ln -f -s $(BASEDIR)/cds.c cds.c
	

cds_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f cds_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
