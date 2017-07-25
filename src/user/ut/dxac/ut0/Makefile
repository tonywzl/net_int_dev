.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = util_nw.c nid_log.c dxac.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h util_nw.c list.h tp_if.h nid_log.c nid_log.h nid_shared.h nid.h \
	umpk_if.h umpk_dxa_if.h umpk_dxp_if.h io_if.h bio_if.h \
	dxa_if.h dxag_if.h dxag.c dxacg_if.h sac_if.h \
	dxac_if.h dxac.c tpg_if.h version.h tx_shared.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk


all: dxac_unit

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

dxa_if.h:
	ln -f -s $(DX_BASEDIR)/dxa_if.h dxa_if.h

dxacg_if.h:
	ln -f -s $(DX_BASEDIR)/dxacg_if.h dxacg_if.h

dxac_if.h:
	ln -f -s $(DX_BASEDIR)/dxac_if.h dxac_if.h

dxac.c:
	ln -f -s $(DX_BASEDIR)/dxac.c dxac.c

dxag_if.h:
	ln -f -s $(DX_BASEDIR)/dxag_if.h dxag_if.h

dxag.c:
	ln -f -s $(DX_BASEDIR)/dxag.c dxag.c

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

dxac_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxac_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
