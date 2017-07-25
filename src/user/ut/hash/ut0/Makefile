.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c hash.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h ini_if.h hash_if.h nid.h  nid_log.c hash.c list.h

BASEDIR = ../../..
MM_BASEDIR = $(BASEDIR)/mm
UMPK_BASEDIR = $(BASEDIR)/umpk

all: hash_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

hash_if.h:
	ln -f -s $(BASEDIR)/hash_if.h hash_if.h	

hash.c:
	ln -f -s $(BASEDIR)/hash.c hash.c

hash_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f hash_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
