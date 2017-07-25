.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c sacg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h nid_shared.h nid.h server.h ini_if.h \
	tp_if.h io_if.h \
	sac_if.h scg_if.h smc_if.h \
	sacg_if.h sacg.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: sacg_unit
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

sacg_if.h:
	ln -f -s $(BASEDIR)/sacg_if.h sacg_if.h

sacg.c:
	ln -f -s $(BASEDIR)/sacg.c sacg.c
	

sacg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sacg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
