.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c smc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h tp_if.h nid_log.c nid_log.h nid_shared.h nid.h \
	mpk_if.h io_if.h bio_if.h \
	smcg_if.h sac_if.h \
	   smc_if.h smc.c

BASEDIR = ../../..
BASESHAREDDIR = ../../../../shared

all: smc_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASESHAREDDIR)/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

mpk_if.h:
	ln -f -s $(BASESHAREDDIR)/mpk_if.h mpk_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

smcg_if.h:
	ln -f -s $(BASEDIR)/smcg_if.h smcg_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

smc.c:
	ln -f -s $(BASEDIR)/smc.c smc.c

smc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f smc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
