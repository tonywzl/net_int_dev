.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dxag.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h nid_log.h nid_log.c list.h ini_if.h nid_shared.h \
	allocator_if.h node_if.h lstn_if.h pp2_if.h \
	dxa_if.h dxt_if.h tpg_if.h \
	dxag_if.h dxag.c \
	umpk_if.h umpk_dxa_if.h \
	tx_shared.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxag_unit

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

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

tx_shared.h:
	ln -f -s $(DX_BASEDIR)/tx_shared.h tx_shared.h

dxa_if.h:
	ln -f -s $(DX_BASEDIR)/dxa_if.h dxa_if.h

dxag_if.h:
	ln -f -s $(DX_BASEDIR)/dxag_if.h dxag_if.h

dxag.c:
	ln -f -s $(DX_BASEDIR)/dxag.c dxag.c

dxt_if.h:
	ln -f -s $(DX_BASEDIR)/dxt_if.h dxt_if.h

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h

dxag_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxag_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
