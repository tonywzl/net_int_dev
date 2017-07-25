.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = nid_log.c  sac.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c nid.h nid_shared.h\
	node_if.h lstn_if.h \
	tp_if.h sah_if.h cds_if.h ds_if.h ini_if.h io_if.h wc_if.h rc_if.h rw_if.h \
	sacg_if.h \
	sac_if.h sac.c

BASEDIR = ../../..

all: sac_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h

cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

sacg_if.h:
	ln -f -s $(BASEDIR)/sacg_if.h sacg_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sac.c:
	ln -f -s ../../../sac.c sac.c

sac_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sac_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
