.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c drv.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c \
	mpk_if.h adt_if.h \
	drv_if.h drv.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: drv_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c


mpk_if.h:
	ln -f -s $(SHARED_BASEDIR)/mpk_if.h mpk_if.h

adt_if.h:
	ln -f -s $(BASEDIR)/adt_if.h adt_if.h

drv_if.h:
	ln -f -s $(BASEDIR)/drv_if.h drv_if.h

drv.c:
	ln -f -s $(BASEDIR)/drv.c drv.c
	

drv_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f drv_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
