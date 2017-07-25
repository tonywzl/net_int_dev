.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c rdg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c \
	rdg_if.h rdg.c

BASEDIR = ../../../..
SHARED_BASEDIR = ../../../../../shared
TESTBASEDIR = ../../..

all: rdg_ut
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

rdg_if.h:
	ln -f -s $(TESTBASEDIR)/rdg_if.h rdg_if.h

rdg.c:
	ln -f -s $(TESTBASEDIR)/rdg.c rdg.c
	

rdg_ut: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f rdg_ut *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
