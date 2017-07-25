.KEEP_STATE:

INCLUDES = -I./ 
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c lck.c tp.c ini.c sds.c cds.c ds.c io.c rio.c bio.c bse.c bfe.c bre.c pp.c ddn.c scg.c sac.c sah.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c nid.h nid_shared.h server.h \
	tp_if.h tp.c \
	ini_if.h ini.c \
	lck_if.h lck.c \
	pp_if.h pp.c \
	ddn_if.h ddn.c \
	rio_if.h rio.c \
	bio_if.h bio.c \
	bse_if.h bse.c \
	bfe_if.h bfe.c \
	bre_if.h bre.c \
	ds_if.h ds.c \
	cds_if.h cds.c \
	sds_if.h sds.c \
	scg_if.h scg.c \
	sac_if.h sac.c \
	sah_if.h sah.c \
	io_if.h io.c

BASEDIR = ../../..

all: io_unit

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

pp.c:
	ln -f -s $(BASEDIR)/mm/pp.c pp.c

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

ddn.c:
	ln -f -s $(BASEDIR)/mm/ddn.c ddn.c

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h

rio.c:
	ln -f -s $(BASEDIR)/rio.c rio.c

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

bio.c:
	ln -f -s $(BASEDIR)/bio.c bio.c

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h

bse.c:
	ln -f -s $(BASEDIR)/bse.c bse.c

bfe_if.h:
	ln -f -s $(BASEDIR)/bfe_if.h bfe_if.h

bfe.c:
	ln -f -s $(BASEDIR)/bfe.c bfe.c

bre_if.h:
	ln -f -s $(BASEDIR)/bre_if.h bre_if.h

bre.c:
	ln -f -s $(BASEDIR)/bre.c bre.c

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h

sds.c:
	ln -f -s $(BASEDIR)/sds.c sds.c
	
cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h

cds.c:
	ln -f -s $(BASEDIR)/cds.c cds.c
	
ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c
	
io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

scg.c:
	ln -f -s $(BASEDIR)/scg.c scg.c
	
sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sac.c:
	ln -f -s $(BASEDIR)/sac.c sac.c
	
sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h

sah.c:
	ln -f -s $(BASEDIR)/sah.c sah.c
	
io.c:
	ln -f -s $(BASEDIR)/io.c io.c
	

io_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f io_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
