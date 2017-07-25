.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  ini.c sah.c ds.c smc.c sac.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid.h nid_shared.h server.h \
	tp_if.h hs_if.h \
	ini_if.h  ini.c \
	sah_if.h  sah.c \
	ds_if.h  ds.c \
	smc_if.h  smc.c \
	scg_if.h io_if.h \
	sac_if.h sac.c

BASEDIR = ../../..

all: sac_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

hs_if.h:
	ln -f -s $(BASEDIR)/hs_if.h hs_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h

sah.c:
	ln -f -s $(BASEDIR)/sah.c sah.c

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

smc.c:
	ln -f -s $(BASEDIR)/smc.c smc.c

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sac.c:
	ln -f -s $(BASEDIR)/sac.c sac.c

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

sac_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sac_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
