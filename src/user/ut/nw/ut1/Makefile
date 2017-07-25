.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  tp.c ini.c ds.c hs.c cc.c sac.c cm.c asc.c acg.c nw.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h server.h list.h nid_log.h \
	tp_if.h tp.c \
	ini_if.h ini.c \
	cc_if.h cc.c \
	sac_if.h sac.c \
	cm_if.h cm.c \
	asc_if.h asc.c \
	acg_if.h acg.c \
	ini_if.h ini.c \
	ds_if.h ds.c \
	hs_if.h hs.c \
	nw_if.h nw.c

BASEDIR = ../../..

all: nw_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
server.h:
	ln -f -s $(BASEDIR)/server.h server.h
list.h:
	ln -f -s $(BASEDIR)/list.h list.h
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h
	
ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h
	
ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c

hs_if.h:
	ln -f -s $(BASEDIR)/hs_if.h hs_if.h
	
hs.c:
	ln -f -s $(BASEDIR)/hs.c hs.c

cc_if.h:
	ln -f -s $(BASEDIR)/cc_if.h cc_if.h

cc.c:
	ln -f -s $(BASEDIR)/cc.c cc.c

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sac.c:
	ln -f -s $(BASEDIR)/sac.c sac.c

cm_if.h:
	ln -f -s $(BASEDIR)/cm_if.h cm_if.h

cm.c:
	ln -f -s $(BASEDIR)/cm.c cm.c

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

asc.c:
	ln -f -s $(BASEDIR)/asc.c asc.c

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

acg.c:
	ln -f -s $(BASEDIR)/acg.c acg.c

nw_if.h:
	ln -f -s $(BASEDIR)/nw_if.h nw_if.h

nw.c:
	ln -f -s $(BASEDIR)/nw.c nw.c

nw_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nw_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
