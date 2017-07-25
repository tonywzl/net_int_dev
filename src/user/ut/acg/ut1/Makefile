.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  tp.c nl.c ini.c ash.c asc.c acg.c amc.c ut_main.c

OBJS = $(SRCS:.c=.o)
BASEDIR = ../../../

SYMLINKS = list.h nid_log.h nid.h agent.h \
	   tp_if.h tp.c \
	   nl_if.h nl.c \
	   ini_if.h ini.c \
	   ash_if.h ash.c \
	   amc_if.h amc.c \
	   asc_if.h asc.c \
	   acg_if.h acg.c

all: acg_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

agent.h:
	ln -f -s $(BASEDIR)/agent.h agent.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

nl.c:
	ln -f -s $(BASEDIR)/nl.c nl.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

ash_if.h:
	ln -f -s $(BASEDIR)/ash_if.h ash_if.h

ash.c:
	ln -f -s $(BASEDIR)/ash.c ash.c

amc_if.h:
	ln -f -s $(BASEDIR)/amc_if.h amc_if.h

amc.c:
	ln -f -s $(BASEDIR)/amc.c amc.c

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

asc.c:
	ln -f -s $(BASEDIR)/asc.c asc.c

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

acg.c:
	ln -f -s $(BASEDIR)/acg.c acg.c

acg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f acg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
