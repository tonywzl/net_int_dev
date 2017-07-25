.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dxpg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.h nid.h nid_log.h nid_log.c nid_shared.h list.h ini_if.h \
	allocator_if.h node_if.h lstn_if.h \
	dxp_if.h tx_shared.h \
	dxpg_if.h dxpg.c \
	umpk_dxa_if.h umpk_dxp_if.h umpk.h umpk_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxpg_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

dxp_if.h:
	ln -f -s $(DX_BASEDIR)/dxp_if.h dxp_if.h

dxpg_if.h:
	ln -f -s $(DX_BASEDIR)/dxpg_if.h dxpg_if.h

dxpg.c:
	ln -f -s $(DX_BASEDIR)/dxpg.c dxpg.c
	
umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk.h:
	ln -f -s $(UMPK_BASEDIR)/umpk.h umpk.h

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h

dxpg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxpg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
