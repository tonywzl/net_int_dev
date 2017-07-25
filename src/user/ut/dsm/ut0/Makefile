.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  dsm.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_shared.h nid_log.h \
	ini_if.h asc_if.h \
	dsm_if.h dsm.c

BASEDIR = ../../..

all: dsm_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h


ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

dsm_if.h:
	ln -f -s $(BASEDIR)/dsm_if.h dsm_if.h

dsm.c:
	ln -f -s $(BASEDIR)/dsm.c dsm.c
	

dsm_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dsm_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
