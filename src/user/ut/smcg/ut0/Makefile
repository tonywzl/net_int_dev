.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c smcg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid.h \
	scg_if.h smc_if.h \
	smcg_if.h smcg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: smcg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

smcg_if.h:
	ln -f -s $(BASEDIR)/smcg_if.h smcg_if.h

smcg.c:
	ln -f -s $(BASEDIR)/smcg.c smcg.c
	

smcg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f smcg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
